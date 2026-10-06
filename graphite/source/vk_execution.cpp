#include "pch.hpp"
#include "graph/execution.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/execution/command_pool.hpp"
#include "vkit/execution/queue.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"
#include "tkit/math/math.hpp"

#define GRAPH_CHECK_QUEUE(queue)                                                                                       \
    TKIT_ASSERT((queue) < Queue_Count && s_Queues[queue],                                                              \
                "[GRAPH][QUEUE] The queue type must be requested at initialization to be used")

namespace Graph
{
struct Vulkan_CommandPool
{
    VKit::CommandPool Pool{};
    TKit::TierArray<CommandBuffer> Commands{};
    u32 NextCommand = 0;
    QueueType Type;
};
struct Vulkan_CommandBuffer
{
    CommandPool Pool;
    VkCommandBuffer Buffer;
};

static TKit::Storage<TKit::ArenaHive<Vulkan_CommandPool>> s_CommandPools{};
static TKit::Storage<TKit::ArenaHive<Vulkan_CommandBuffer>> s_CommandBuffers{};
static TKit::FixedArray<VKit::Queue *, Queue_Count> s_Queues{};

void Execution_Initialize(const u32 maxPools, const u32 maxCmdBuffers)
{
    s_CommandPools.Construct();
    s_CommandBuffers.Construct();

    s_CommandPools->Reserve(maxPools);
    s_CommandBuffers->Reserve(maxCmdBuffers);

    const auto &device = GetDevice();
    const auto table = GetDeviceTable();

    const auto &perType = device.GetInfo().QueuesPerType;
    s_Queues[Queue_Graphics] = perType[VKit::Queue_Graphics].GetFront();
    s_Queues[Queue_Transfer] = perType[VKit::Queue_Transfer].GetFront();
    s_Queues[Queue_Compute] = perType[VKit::Queue_Compute].GetFront();

    const auto &queues = device.GetInfo().Queues;
    u32 i = 0;
    for (VKit::Queue *q : queues)
    {
        VkSemaphoreTypeCreateInfoKHR typeInfo{};
        typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        typeInfo.initialValue = 0;

        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = &typeInfo;
        VkSemaphore semaphore;

        GRAPH_CHECK_RESULT(table->CreateSemaphore(device, &info, nullptr, &semaphore));
        q->TakeTimelineSemaphoreOwnership(semaphore);
        GRAPH_CHECK_RESULT(q->UpdateCompletedTimelineValues());
        if (IsDebugUtilsEnabled())
        {
            const TKit::StackString name =
                TKit::StackString::Format("graph-timeline-semaphore-queue-index-{}-family-{}", i++, q->GetFamily());
            GRAPH_CHECK_RESULT(device.SetObjectName(semaphore, VK_OBJECT_TYPE_SEMAPHORE, name.CString()));
        }
    }
}

void Execution_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_CommandPools, "EXECUTION", "command pools", Pool.Destroy());

    s_Queues = {};
    s_CommandBuffers.Destruct();
    s_CommandPools.Destruct();
}

bool BelongToTheSameFamily(const QueueType type0, const QueueType type1)
{
    const auto &indices = GetPhysical().GetInfo().FamilyIndices;
    return indices[ToVulkan(type0)] == indices[ToVulkan(type1)];
}

u64 Queue_GetCompletedTimelineValues(const QueueType queue)
{
    GRAPH_CHECK_QUEUE(queue);
    return s_Queues[queue]->GetCompletedTimelineValues();
}
u64 Queue_GetSubmittedTimelineValues(const QueueType queue)
{
    GRAPH_CHECK_QUEUE(queue);
    return s_Queues[queue]->GetSubmittedTimelineValues();
}
u64 Queue_ReserveTimelineValue(const QueueType queue)
{
    GRAPH_CHECK_QUEUE(queue);
    return s_Queues[queue]->ReserveTimelineValue();
}
u64 Queue_UpdateCompletedTimelineValues(const QueueType queue)
{
    GRAPH_CHECK_QUEUE(queue);
    return GRAPH_CHECK_RESULT(s_Queues[queue]->UpdateCompletedTimelineValues());
}
void UpdateCompletedTimelineValuesForAllQueues()
{
    for (VKit::Queue *q : GetDevice().GetInfo().Queues)
    {
        GRAPH_CHECK_RESULT(q->UpdateCompletedTimelineValues());
    }
}
void Queue_SetName(const QueueType queue, const char *name)
{
    GRAPH_CHECK_QUEUE(queue);
    GRAPH_CHECK_RESULT(s_Queues[queue]->SetName(name));
}
bool Queue_IsHandleValid(const QueueType queue)
{
    return queue < Queue_Count && s_Queues[queue];
}

void Queue_Submit(const QueueType queue, const TKit::Span<const SubmitInfo> infos)
{
    GRAPH_CHECK_QUEUE(queue);
    VKit::Queue *vkqueue = s_Queues[queue];

    u32 commandCount = 0;
    u32 wcount = 0;
    for (const SubmitInfo &info : infos)
    {
        commandCount += info.Commands.GetSize();
        wcount += info.Waits.GetSize();
    }

    TKit::StackArray<VkCommandBufferSubmitInfoKHR> commands{};
    TKit::StackArray<VkSemaphoreSubmitInfoKHR> waits{};
    TKit::StackArray<VkSemaphoreSubmitInfoKHR> signals{};
    TKit::StackArray<VkSubmitInfo2KHR> vkinfos{};
    commands.Reserve(commandCount);
    waits.Reserve(wcount + infos.GetSize());
    signals.Reserve(infos.GetSize() * 2);
    vkinfos.Reserve(infos.GetSize());

    for (const SubmitInfo &info : infos)
    {
        VkSubmitInfo2KHR vkinfo{};
        vkinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;

        if (!info.Commands.IsEmpty())
        {
            vkinfo.pCommandBufferInfos = commands.GetData() + commands.GetSize();
            vkinfo.commandBufferInfoCount = info.Commands.GetSize();
            for (const CommandBuffer cmd : info.Commands)
            {
                GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
                VkCommandBufferSubmitInfoKHR cinfo{};
                cinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
                cinfo.commandBuffer = s_CommandBuffers->At(Handle_GetId(cmd)).Buffer;
                commands.Append(cinfo);
            }
        }

        u32 waitCount = 0;
        const VkSemaphoreSubmitInfoKHR *waitPtr = waits.end();
        if (!info.Waits.IsEmpty())
        {
            waitCount += info.Waits.GetSize();
            for (const WaitInfo &wait : info.Waits)
            {
                GRAPH_CHECK_QUEUE(wait.QueueType);
                VkSemaphoreSubmitInfoKHR winfo{};
                winfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
                winfo.semaphore = s_Queues[wait.QueueType]->GetTimelineSempahore();
                winfo.value = wait.Value;
                winfo.stageMask = ToVulkanPipelineStageFlags(wait.StageFlags);
                waits.Append(winfo);
            }
        }

        u32 signalCount = 0;
        const VkSemaphoreSubmitInfoKHR *signalPtr = signals.end();
        if (info.SignalValue)
        {
            TKIT_ASSERT(info.SignalValue > vkqueue->GetSubmittedTimelineValues() &&
                            info.SignalValue <= vkqueue->GetLastReservedTimelineValue(),
                        "[GRAPH][QUEUE] SignalValue must be a value reserved with Queue_ReserveTimelineValue() that "
                        "has not been submitted yet");
            VkSemaphoreSubmitInfoKHR sinfo{};
            sinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
            sinfo.semaphore = vkqueue->GetTimelineSempahore();
            sinfo.value = info.SignalValue;
            sinfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
            signals.Append(sinfo);
            ++signalCount;
        }

#ifdef GRAPH_HAS_PLATFORM_BACKEND
        if (info.Swapchain != NullHandle)
        {
            TKIT_ASSERT(info.SignalValue != 0, "[GRAPH][EXECUTION] If a swap chain is provided for synchronization, a "
                                               "non-zero signal value must be provided");
            const VkSemaphore rsem = GetRenderFinishedSemaphore(info.Swapchain);
            const VkSemaphore imgav = GetImageAvailableSemaphore(info.Swapchain);

            VkSemaphoreSubmitInfoKHR wait{};
            wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
            wait.semaphore = imgav;
            wait.stageMask = ToVulkanPipelineStageFlags(info.SwapchainWaitStageFlags);
            waits.Append(wait);

            VkSemaphoreSubmitInfoKHR signal{};
            signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
            signal.semaphore = rsem;
            wait.stageMask = ToVulkanPipelineStageFlags(info.SwapchainSignalStageFlags);
            signals.Append(signal);

            ++waitCount;
            ++signalCount;
            SetRenderTimelineTracker(info.Swapchain, {info.SignalValue, queue});
        }
#endif
        vkinfo.pWaitSemaphoreInfos = waitPtr;
        vkinfo.waitSemaphoreInfoCount = waitCount;
        vkinfo.pSignalSemaphoreInfos = signalPtr;
        vkinfo.signalSemaphoreInfoCount = signalCount;
        vkinfos.Append(vkinfo);
    }
    GRAPH_CHECK_RESULT(vkqueue->Submit2(vkinfos));
}

bool Queue_WaitForTimelineValue(const QueueType queue, const u64 value, const u64 timeout)
{
    GRAPH_CHECK_QUEUE(queue);
    VKit::Queue *vkqueue = s_Queues[queue];
    TKIT_ASSERT(value <= vkqueue->GetSubmittedTimelineValues(),
                "[GRAPH][QUEUE] Cannot wait for a timeline value that has not been submitted");

    const VkSemaphore semaphore = vkqueue->GetTimelineSempahore();
    VkSemaphoreWaitInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;
    info.semaphoreCount = 1;
    info.pSemaphores = &semaphore;
    info.pValues = &value;

    if (GetDeviceTable()->WaitSemaphoresKHR(GetDevice(), &info, timeout) != VK_SUCCESS)
        return false;

    GRAPH_CHECK_RESULT(vkqueue->UpdateCompletedTimelineValues());
    return true;
}

CommandPool CommandPool_Create(const QueueType type, const CommandPoolFlags flags)
{
    const u32 family = GetPhysical().GetInfo().FamilyIndices[type];
    const Id id = s_CommandPools->Insert();
    Vulkan_CommandPool &pool = s_CommandPools->At(id);

    VkCommandPoolCreateFlags vkFlags = 0;
    if (flags & CommandPoolFlag_CreateTransient)
        vkFlags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    pool.Pool = GRAPH_CHECK_RESULT(VKit::CommandPool::Create(GetDevice(), family, vkFlags));
    pool.Type = type;
    return Handle_Create(Handle_CommandPool, id);
}
void CommandPool_Destroy(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);

    const TKit::StackArray<Id> cmdBufIds = s_CommandBuffers->GetValidIds();
    for (const Id id : cmdBufIds)
        if (s_CommandBuffers->At(id).Pool == pool)
            s_CommandBuffers->Remove(id);

    GRAPH_DESTROY_FUNCTION_BODY_ACCESSOR(s_CommandPools, pool, Pool.Destroy());
}
QueueType CommandPool_GetQueueType(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);
    return s_CommandPools->At(Handle_GetId(pool)).Type;
}
void CommandPool_Reset(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);
    Vulkan_CommandPool &p = s_CommandPools->At(Handle_GetId(pool));
    GRAPH_CHECK_RESULT(p.Pool.Reset());
    p.NextCommand = 0;
}
static CommandBuffer commandPool_CreateCommand(const CommandPool pool, const VkCommandBuffer vkcmd)
{
    const Id cmdId = s_CommandBuffers->Insert();
    Vulkan_CommandBuffer &cmd = s_CommandBuffers->At(cmdId);
    cmd.Pool = pool;
    cmd.Buffer = vkcmd;

    return Handle_Create(Handle_CommandBuffer, cmdId);
}
CommandBuffer CommandPool_BeginImmediateSubmission(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);
    return commandPool_CreateCommand(
        pool, GRAPH_CHECK_RESULT(s_CommandPools->At(Handle_GetId(pool)).Pool.BeginSingleTimeCommands()));
}
void CommandPool_EndImmediateSubmission(const CommandPool pool, const CommandBuffer cmd, const QueueType queue)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GRAPH_CHECK_QUEUE(queue);

    const Id id = Handle_GetId(cmd);
    GRAPH_CHECK_RESULT(s_CommandPools->At(Handle_GetId(pool))
                           .Pool.EndSingleTimeCommands(s_CommandBuffers->At(id).Buffer, *s_Queues[queue]));

    s_CommandBuffers->Remove(id);
}

CommandBuffer CommandPool_NextCommandBuffer(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);

    const Id id = Handle_GetId(pool);
    Vulkan_CommandPool &p = s_CommandPools->At(id);

    if (p.NextCommand < p.Commands.GetSize())
        return p.Commands[p.NextCommand++];

    p.NextCommand++;
    return commandPool_CreateCommand(pool, GRAPH_CHECK_RESULT(p.Pool.Allocate()));
}

void CommandPool_SetName(const CommandPool pool, const char *name)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);
    TKIT_ASSERT(IsValidationEnabled(), "[GRAPH][EXECUTION] To name objects, the validation capability must be enabled");
    GRAPH_CHECK_RESULT(s_CommandPools->At(Handle_GetId(pool)).Pool.SetName(name));
}

bool CommandPool_IsHandleValid(const CommandPool pool)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_CommandPools, pool, Handle_CommandPool);
}

void CommandBuffer_Begin(const CommandBuffer cmd)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    GRAPH_CHECK_RESULT(GetDeviceTable()->BeginCommandBuffer(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, &info));
}

void CommandBuffer_End(const CommandBuffer cmd)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GRAPH_CHECK_RESULT(GetDeviceTable()->EndCommandBuffer(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer));
}

bool CommandBuffer_IsHandleValid(const CommandBuffer cmd)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_CommandBuffers, cmd, Handle_CommandBuffer);
}

static VkRenderingAttachmentInfoKHR createAttachmentInfo(const AttachmentInfo &att, const bool color)
{
    VkRenderingAttachmentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    info.imageView = GetImageView(att.View);
    info.imageLayout = ToVulkan(att.Layout);
    info.loadOp = ToVulkan(att.LoadOp);
    info.storeOp = ToVulkan(att.StoreOp);
    if (color)
        for (u32 i = 0; i < 4; ++i)
            info.clearValue.color.float32[i] = att.ClearValue.Color[i];
    else
    {
        info.clearValue.depthStencil.depth = att.ClearValue.DepthStencil.Depth;
        info.clearValue.depthStencil.stencil = att.ClearValue.DepthStencil.Stencil;
    }
    if (att.Resolve.View != NullHandle)
    {
        info.resolveMode = ToVulkan(att.Resolve.Mode);
        info.resolveImageView = GetImageView(att.Resolve.View);
        info.resolveImageLayout = ToVulkan(att.Resolve.Layout);
    }
    return info;
}

static VkExtent3D getMipExtent(const VKit::DeviceImage &image, const u32 mip)
{
    const auto &info = image.GetInfo();
    return {TKit::Math::Max(info.Width >> mip, 1u), TKit::Math::Max(info.Height >> mip, 1u),
            TKit::Math::Max(info.Depth >> mip, 1u)};
}

static VkExtent3D resolveExtent(const u32v3 &v, const VkExtent3D &mip)
{
    return {v[0] == GRAPH_WHOLE_THING ? mip.width : v[0], v[1] == GRAPH_WHOLE_THING ? mip.height : v[1],
            v[2] == GRAPH_WHOLE_THING ? mip.depth : v[2]};
}

static VkOffset3D toOffset(const VkExtent3D &e)
{
    return {i32(e.width), i32(e.height), i32(e.depth)};
}

struct BarrierMasks
{
    VkPipelineStageFlags2KHR SrcStages = 0;
    VkAccessFlags2KHR SrcAccess = 0;
    VkPipelineStageFlags2KHR DstStages = 0;
    VkAccessFlags2KHR DstAccess = 0;
    u32 SrcFamily = VK_QUEUE_FAMILY_IGNORED;
    u32 DstFamily = VK_QUEUE_FAMILY_IGNORED;
    bool Release = false;
};

template <typename T> static BarrierMasks createBarrierMasks(const QueueType cmdQueue, const T &b)
{
    const bool release = b.HandTo != Queue_None && !BelongToTheSameFamily(b.HandTo, cmdQueue);
    const bool acquire = b.AcceptFrom != Queue_None && !BelongToTheSameFamily(b.AcceptFrom, cmdQueue);
    TKIT_ASSERT(!acquire || !release, "[GRAPH][COMMAND] A barrier cannot both hand over and accept ownership");

    BarrierMasks m{};
    const auto &indices = GetPhysical().GetInfo().FamilyIndices;
    if (acquire)
    {
        m.SrcFamily = indices[ToVulkan(b.AcceptFrom)];
        m.DstFamily = indices[ToVulkan(cmdQueue)];
    }
    else if (release)
    {
        m.SrcFamily = indices[ToVulkan(cmdQueue)];
        m.DstFamily = indices[ToVulkan(b.HandTo)];
        m.Release = true;
    }
    m.SrcStages = ToVulkanPipelineStageFlags(b.SrcStages);
    m.SrcAccess = ToVulkanAccessFlags(b.SrcAccess);
    m.DstStages = ToVulkanPipelineStageFlags(b.DstStages);
    m.DstAccess = ToVulkanAccessFlags(b.DstAccess);
    return m;
}

void Command_BeginRenderPass(const CommandBuffer cmd, const RenderPassInfo &info)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    TKit::StackArray<VkRenderingAttachmentInfoKHR> colors{};
    colors.Reserve(info.ColorAttachments.GetSize());
    for (const AttachmentInfo &att : info.ColorAttachments)
        colors.Append(createAttachmentInfo(att, true));

    VkRenderingAttachmentInfoKHR depth{};
    VkRenderingAttachmentInfoKHR stencil{};

    VkRenderingInfoKHR vkinfo{};
    vkinfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

    const Rect &r = info.RenderArea;
    vkinfo.renderArea = {{r.Offset[0], r.Offset[1]}, {r.Extent[0], r.Extent[1]}};
    vkinfo.layerCount = info.LayerCount;
    vkinfo.colorAttachmentCount = colors.GetSize();
    vkinfo.pColorAttachments = colors.GetData();
    if (info.DepthAttachment)
    {
        depth = createAttachmentInfo(*info.DepthAttachment, false);
        vkinfo.pDepthAttachment = &depth;
    }
    if (info.StencilAttachment)
    {
        stencil = createAttachmentInfo(*info.StencilAttachment, false);
        vkinfo.pStencilAttachment = &stencil;
    }
    GetDeviceTable()->CmdBeginRenderingKHR(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, &vkinfo);
}

void Command_EndRenderPass(const CommandBuffer cmd)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdEndRenderingKHR(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer);
}

void Command_PipelineBarrier(const CommandBuffer cmd, const PipelineBarrierInfo &info)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    const Vulkan_CommandBuffer &vkcmd = s_CommandBuffers->At(Handle_GetId(cmd));
    const QueueType cmdQueue = s_CommandPools->At(Handle_GetId(vkcmd.Pool)).Type;

    TKit::StackArray<VkMemoryBarrier2KHR> memory{};
    TKit::StackArray<VkBufferMemoryBarrier2KHR> buffers{};
    TKit::StackArray<VkImageMemoryBarrier2KHR> images{};
    memory.Reserve(info.MemoryBarriers.GetSize());
    buffers.Reserve(info.BufferBarriers.GetSize());
    images.Reserve(info.ImageBarriers.GetSize());

    for (const MemoryBarrierInfo &b : info.MemoryBarriers)
    {
        VkMemoryBarrier2KHR vk{};
        vk.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
        vk.srcStageMask = ToVulkanPipelineStageFlags(b.SrcStages);
        vk.srcAccessMask = ToVulkanAccessFlags(b.SrcAccess);
        vk.dstStageMask = ToVulkanPipelineStageFlags(b.DstStages);
        vk.dstAccessMask = ToVulkanAccessFlags(b.DstAccess);
        memory.Append(vk);
    }
    for (const BufferMemoryBarrierInfo &b : info.BufferBarriers)
    {
        GRAPH_CHECK_HANDLE(b.Handle, Handle_Buffer);
        const BarrierMasks m = createBarrierMasks(cmdQueue, b);
        VkBufferMemoryBarrier2KHR vk{};
        vk.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
        vk.srcStageMask = m.SrcStages;
        vk.srcAccessMask = m.SrcAccess;
        vk.dstStageMask = m.DstStages;
        vk.dstAccessMask = m.DstAccess;
        vk.srcQueueFamilyIndex = m.SrcFamily;
        vk.dstQueueFamilyIndex = m.DstFamily;
        vk.buffer = GetBuffer(b.Handle);
        vk.offset = b.Offset;
        vk.size = b.Size ? b.Size : VK_WHOLE_SIZE;
        buffers.Append(vk);
    }
    for (const ImageMemoryBarrierInfo &b : info.ImageBarriers)
    {
        GRAPH_CHECK_HANDLE(b.Handle, Handle_Image);
        const BarrierMasks m = createBarrierMasks(cmdQueue, b);
        VKit::DeviceImage &image = GetImage(b.Handle);
        const VkImageLayout layout = b.NewLayout == ImageLayout_Undefined ? image.Layout : ToVulkan(b.NewLayout);
        images.Append(image.CreateTransitionLayoutBarrier2(layout, {.SrcFamilyIndex = m.SrcFamily,
                                                                    .DstFamilyIndex = m.DstFamily,
                                                                    .SrcAccess = m.SrcAccess,
                                                                    .DstAccess = m.DstAccess,
                                                                    .SrcStage = m.SrcStages,
                                                                    .DstStage = m.DstStages,
                                                                    .Range = ToVulkan(image, b.Range)}));
        if (!m.Release)
            image.Layout = layout;
    }

    VkDependencyInfoKHR dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
    dep.memoryBarrierCount = memory.GetSize();
    dep.pMemoryBarriers = memory.GetData();
    dep.bufferMemoryBarrierCount = buffers.GetSize();
    dep.pBufferMemoryBarriers = buffers.GetData();
    dep.imageMemoryBarrierCount = images.GetSize();
    dep.pImageMemoryBarriers = images.GetData();
    GetDeviceTable()->CmdPipelineBarrier2KHR(vkcmd.Buffer, &dep);
}

void Command_BindPipeline(const CommandBuffer cmd, const Pipeline pip)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    BindPipeline(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, pip);
}

void Command_BindDescriptorSets(const CommandBuffer cmd, const PipelineLayout layout, const BindPoint bpoint,
                                const TKit::Span<const DescriptorSet> sets, const u32 firstSet)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    TKit::StackArray<VkDescriptorSet> vksets{};
    vksets.Reserve(sets.GetSize());
    for (const DescriptorSet set : sets)
        vksets.Append(GetDescriptorSet(set));

    GetDeviceTable()->CmdBindDescriptorSets(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, ToVulkan(bpoint),
                                            GetPipelineLayout(layout), firstSet, vksets.GetSize(), vksets.GetData(), 0,
                                            nullptr);
}

void Command_BindIndexBuffer(const CommandBuffer cmd, const Buffer buffer, const IndexType type)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdBindIndexBuffer(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, GetBuffer(buffer), 0,
                                         ToVulkan(type));
}

void Command_BindVertexBuffers(const CommandBuffer cmd, const TKit::Span<const Buffer> buffers, const u32 firstBinding)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    TKit::StackArray<VkBuffer> vkbuffers{};
    TKit::StackArray<VkDeviceSize> offsets{};
    vkbuffers.Reserve(buffers.GetSize());
    offsets.Reserve(buffers.GetSize());
    for (const Buffer buffer : buffers)
    {
        vkbuffers.Append(GetBuffer(buffer));
        offsets.Append(0);
    }
    GetDeviceTable()->CmdBindVertexBuffers(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, firstBinding,
                                           vkbuffers.GetSize(), vkbuffers.GetData(), offsets.GetData());
}

void Command_PushConstants(const CommandBuffer cmd, const PipelineLayout layout, const ShaderStageFlags stages,
                           const void *values, const u32 size, const u32 offset)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdPushConstants(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, GetPipelineLayout(layout),
                                       ToVulkanShaderStageFlags(stages), offset, size, values);
}

void Command_SetViewport(const CommandBuffer cmd, const f32v2 &pos, const f32v2 &size, const f32v2 &depth)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    const VkViewport viewport{pos[0], pos[1], size[0], size[1], depth[0], depth[1]};
    GetDeviceTable()->CmdSetViewport(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, 0, 1, &viewport);
}

void Command_SetScissor(const CommandBuffer cmd, const i32v2 &pos, const u32v2 &size)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    const VkRect2D scissor{{pos[0], pos[1]}, {size[0], size[1]}};
    GetDeviceTable()->CmdSetScissor(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, 0, 1, &scissor);
}

void Command_SetCullMode(const CommandBuffer cmd, const CullMode cullMode)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdSetCullModeEXT(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, ToVulkan(cullMode));
}

void Command_Draw(const CommandBuffer cmd, const u32 vertexCount, const u32 instanceCount, const u32 firstVertex,
                  const u32 firstInstance)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdDraw(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, vertexCount, instanceCount, firstVertex,
                              firstInstance);
}

void Command_DrawIndexed(const CommandBuffer cmd, const u32 indexCount, const u32 instanceCount, const u32 firstIndex,
                         const u32 firstInstance, const u32 vertexOffset)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdDrawIndexed(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, indexCount, instanceCount,
                                     firstIndex, i32(vertexOffset), firstInstance);
}

static_assert(sizeof(DrawIndirectCommand) == sizeof(VkDrawIndirectCommand));
static_assert(sizeof(DrawIndexedIndirectCommand) == sizeof(VkDrawIndexedIndirectCommand));

void Command_DrawIndirect(const CommandBuffer cmd, const Buffer buffer, const u32 drawCount, const usz offset)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdDrawIndirect(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, GetBuffer(buffer), offset,
                                      drawCount, sizeof(VkDrawIndirectCommand));
}

void Command_DrawIndexedIndirect(const CommandBuffer cmd, const Buffer buffer, const u32 drawCount, const usz offset)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdDrawIndexedIndirect(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, GetBuffer(buffer), offset,
                                             drawCount, sizeof(VkDrawIndexedIndirectCommand));
}

void Command_Dispatch(const CommandBuffer cmd, const u32 groupsX, const u32 groupsY, const u32 groupsZ)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    GetDeviceTable()->CmdDispatch(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, groupsX, groupsY, groupsZ);
}

void Command_UpdateBuffer(const CommandBuffer cmd, const Buffer buffer, const void *data, const usz size,
                          const usz offset)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    TKIT_ASSERT(size <= 65536 && size % 4 == 0 && offset % 4 == 0,
                "[GRAPH][COMMAND] Buffer updates must be at most 65536 bytes, and size and offset must be multiples of "
                "4. Size: {}, offset: {}",
                size, offset);
    GetDeviceTable()->CmdUpdateBuffer(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, GetBuffer(buffer), offset, size,
                                      data);
}

void Command_CopyBuffer(const CommandBuffer cmd, const Buffer src, const Buffer dst,
                        const TKit::Span<const BufferCopy> regions)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);

    TKit::StackArray<VkBufferCopy2KHR> copies{};
    copies.Reserve(regions.GetSize());
    for (const BufferCopy &r : regions)
    {
        VkBufferCopy2KHR c{};
        c.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2_KHR;
        c.srcOffset = r.SrcOffset;
        c.dstOffset = r.DstOffset;
        c.size = r.Size;
        copies.Append(c);
    }
    VkCopyBufferInfo2KHR copyInfo{};
    copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2_KHR;
    copyInfo.srcBuffer = GetBuffer(src);
    copyInfo.dstBuffer = GetBuffer(dst);
    copyInfo.regionCount = copies.GetSize();
    copyInfo.pRegions = copies.GetData();
    GetDeviceTable()->CmdCopyBuffer2KHR(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, &copyInfo);
}

void Command_CopyBufferToImage(const CommandBuffer cmd, const Buffer src, const Image dst,
                               const TKit::Span<const BufferImageCopy> regions)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    VKit::DeviceImage &image = GetImage(dst);

    TKit::StackArray<VkBufferImageCopy2KHR> copies{};
    copies.Reserve(regions.GetSize());
    for (const BufferImageCopy &r : regions)
    {
        VkBufferImageCopy2KHR c{};
        c.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2_KHR;
        c.bufferOffset = r.BufferOffset;
        c.bufferRowLength = r.BufferRowLength;
        c.bufferImageHeight = r.BufferImageHeight;
        c.imageSubresource = ToVulkan(image, r.Layers);
        c.imageOffset = {i32(r.ImageOffset[0]), i32(r.ImageOffset[1]), i32(r.ImageOffset[2])};
        c.imageExtent = resolveExtent(r.ImageExtent, getMipExtent(image, r.Layers.MipLevel));
        copies.Append(c);
    }
    VkCopyBufferToImageInfo2KHR copyInfo{};
    copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2_KHR;
    copyInfo.srcBuffer = GetBuffer(src);
    copyInfo.dstImage = image;
    copyInfo.dstImageLayout = image.Layout;
    copyInfo.regionCount = copies.GetSize();
    copyInfo.pRegions = copies.GetData();
    GetDeviceTable()->CmdCopyBufferToImage2KHR(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, &copyInfo);
}

void Command_BlitImage(const CommandBuffer cmd, const Image src, const Image dst,
                       const TKit::Span<const ImageBlit> regions, const Filter filter)
{
    GRAPH_CHECK_HANDLE(cmd, Handle_CommandBuffer);
    VKit::DeviceImage &simg = GetImage(src);
    VKit::DeviceImage &dimg = GetImage(dst);

    TKit::StackArray<VkImageBlit2KHR> blits{};
    blits.Reserve(regions.GetSize());
    for (const ImageBlit &r : regions)
    {
        const VkExtent3D smip = getMipExtent(simg, r.SrcMip);
        const VkExtent3D dmip = getMipExtent(dimg, r.DstMip);

        VkImageBlit2KHR b{};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
        b.srcSubresource = {ToVulkanImageAspectFlags(simg, r.Aspect), r.SrcMip, r.SrcLayerStart, r.LayerCount};
        b.srcOffsets[0] = toOffset(resolveExtent(r.SrcMin, smip));
        b.srcOffsets[1] = toOffset(resolveExtent(r.SrcMax, smip));
        b.dstSubresource = {ToVulkanImageAspectFlags(dimg, r.Aspect), r.DstMip, r.DstLayerStart, r.LayerCount};
        b.dstOffsets[0] = toOffset(resolveExtent(r.DstMin, dmip));
        b.dstOffsets[1] = toOffset(resolveExtent(r.DstMax, dmip));
        blits.Append(b);
    }
    VkBlitImageInfo2KHR blitInfo{};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2_KHR;
    blitInfo.srcImage = simg;
    blitInfo.srcImageLayout = simg.Layout;
    blitInfo.dstImage = dimg;
    blitInfo.dstImageLayout = dimg.Layout;
    blitInfo.regionCount = blits.GetSize();
    blitInfo.pRegions = blits.GetData();
    blitInfo.filter = ToVulkan(filter);
    GetDeviceTable()->CmdBlitImage2KHR(s_CommandBuffers->At(Handle_GetId(cmd)).Buffer, &blitInfo);
}
} // namespace Graph
