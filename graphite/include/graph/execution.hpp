#pragma once

#include "graph/resources.hpp"
#include "tkit/math/tensor.hpp"
#include "tkit/container/span.hpp"

namespace Graph
{
using CommandPoolFlags = u8;
enum CommandPoolFlagBit : u8
{
    CommandPoolFlag_CreateTransient = 1U << 0,
};

bool BelongToTheSameFamily(QueueType type0, QueueType type1);

Queue Queue_Get(QueueType type);
QueueType Queue_GetType(Queue queue);
u64 Queue_GetCompletedTimeline(Queue queue);
u64 Queue_GetTimelineSubmissions(Queue queue);

void Queue_SetName(Queue queue, const char *name);
bool Queue_IsHandleValid(Queue queue);

CommandPool CommandPool_Create(QueueType type, CommandPoolFlags flags = 0);
void CommandPool_Destroy(CommandPool pool);
void CommandPool_Reset(CommandPool pool);

CommandBuffer CommandPool_BeginImmediateSubmission(CommandPool pool);
void CommandPool_EndImmediateSubmission(CommandPool pool, CommandBuffer cmd, Queue queue);

template <typename F> void CommandPool_ImmediateSubmission(const CommandPool pool, const Queue queue, F &&fun)
{
    const CommandBuffer cmd = CommandPool_BeginImmediateSubmission(pool);
    std::forward<F>(fun)(cmd);
    CommandPool_EndImmediateSubmission(pool, cmd, queue);
}

CommandBuffer CommandPool_NextCommandBuffer(CommandPool pool);

void CommandPool_SetName(CommandPool pool, const char *name);
bool CommandPool_IsHandleValid(CommandPool pool);

bool CommandBuffer_IsHandleValid(CommandBuffer cmd);

struct Rect
{
    i32v2 Offset{0};
    u32v2 Extent{0};
};

struct ResolveInfo
{
    ImageView View = NullHandle;
    ImageLayout Layout = ImageLayout_Undefined;
    ResolveMode Mode = Resolve_Average;
};

struct ClearDepthStencil
{
    f32 Depth = 1.f;
    u32 Stencil = 0;
};

struct ClearInfo
{
    f32v4 Color{0.f};
    ClearDepthStencil DepthStencil{};
};

struct AttachmentInfo
{
    ImageView View = NullHandle;
    ResolveInfo Resolve{};
    ImageLayout Layout = ImageLayout_Undefined;
    ClearInfo ClearValue{};
    LoadOp LoadOp = LoadOp_DontCare;
    StoreOp StoreOp = StoreOp_DontCare;
};

struct RenderPassInfo
{
    Rect RenderArea{};
    u32 LayerCount = 1;
    TKit::Span<const AttachmentInfo> ColorAttachments{};
    const AttachmentInfo *DepthAttachment = nullptr;
    const AttachmentInfo *StencilAttachment = nullptr;
};

struct MemoryBarrierInfo
{
    PipelineStageFlags SrcStages = 0;
    AccessFlags SrcAccess = 0;
    PipelineStageFlags DstStages = 0;
    AccessFlags DstAccess = 0;
};

struct BufferMemoryBarrierInfo
{
    usz Offset = 0;
    usz Size = 0;
    Buffer Handle = NullHandle;
    QueueType HandTo = Queue_None;
    QueueType AcceptFrom = Queue_None;
    PipelineStageFlags SrcStages = 0;
    AccessFlags SrcAccess = 0;
    PipelineStageFlags DstStages = 0;
    AccessFlags DstAccess = 0;
};

struct ImageMemoryBarrierInfo
{
    Image Handle = NullHandle;
    ImageSubresourceRange Range{};
    ImageLayout NewLayout = ImageLayout_Undefined;
    QueueType HandTo = Queue_None;
    QueueType AcceptFrom = Queue_None;
    PipelineStageFlags SrcStages = 0;
    AccessFlags SrcAccess = 0;
    PipelineStageFlags DstStages = 0;
    AccessFlags DstAccess = 0;
};

struct PipelineBarrierInfo
{
    TKit::Span<const MemoryBarrierInfo> MemoryBarriers{};
    TKit::Span<const BufferMemoryBarrierInfo> BufferBarriers{};
    TKit::Span<const ImageMemoryBarrierInfo> ImageBarriers{};
};

struct DrawIndirectCommand
{
    u32 VertexCount = 0;
    u32 InstanceCount = 1;
    u32 FirstVertex = 0;
    u32 FirstInstance = 0;
};

struct DrawIndexedIndirectCommand
{
    u32 IndexCount = 0;
    u32 InstanceCount = 1;
    u32 FirstIndex = 0;
    i32 VertexOffset = 0;
    u32 FirstInstance = 0;
};

void Command_BeginRenderPass(CommandBuffer cmd, const RenderPassInfo &info);
void Command_EndRenderPass(CommandBuffer cmd);

void Command_PipelineBarrier(CommandBuffer cmd, const PipelineBarrierInfo &info);
void Command_MemoryBarrier(const CommandBuffer cmd, const TKit::Span<const MemoryBarrierInfo> barriers)
{
    return Command_PipelineBarrier(cmd, {.MemoryBarriers = barriers});
}
void Command_BufferBarrier(const CommandBuffer cmd, const TKit::Span<const BufferMemoryBarrierInfo> barriers)
{
    return Command_PipelineBarrier(cmd, {.BufferBarriers = barriers});
}
void Command_ImageBarrier(const CommandBuffer cmd, const TKit::Span<const ImageMemoryBarrierInfo> barriers)
{
    return Command_PipelineBarrier(cmd, {.ImageBarriers = barriers});
}

void Command_BindPipeline(CommandBuffer cmd, Pipeline pip);
void Command_BindDescriptorSets(CommandBuffer cmd, PipelineLayout layout, BindPoint bpoint,
                                TKit::Span<const DescriptorSet> sets, u32 firstSet = 0);

void Command_BindIndexBuffer(CommandBuffer cmd, Buffer buffer, IndexType type = IndexType_Unsigned16);
void Command_BindVertexBuffers(CommandBuffer cmd, TKit::Span<const Buffer> buffers, u32 firstBinding = 0);
void Command_PushConstants(CommandBuffer cmd, PipelineLayout layout, ShaderStageFlags stages, const void *values,
                           u32 size, u32 offset = 0);
template <typename T>
void Command_PushConstants(CommandBuffer cmd, PipelineLayout layout, ShaderStageFlags stages, const T *values,
                           u32 offset = 0)
{
    Command_PushConstants(cmd, layout, stages, values, sizeof(T), offset);
}

void Command_SetViewport(CommandBuffer cmd, const f32v2 &pos, const f32v2 &size, const f32v2 &depth = {0.f, 1.f});
void Command_SetScissor(CommandBuffer cmd, const Rect &rect);
void Command_SetCullMode(CommandBuffer cmd, CullMode cullMode);
void Command_Draw(CommandBuffer cmd, u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0,
                  u32 firstInstance = 0);
void Command_DrawIndexed(CommandBuffer cmd, u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0,
                         u32 firstInstance = 0, u32 vertexOffset = 0);

void Command_DrawIndirect(CommandBuffer cmd, Buffer buffer, u32 drawCount = 1, usz offset = 0);
void Command_DrawIndexedIndirect(CommandBuffer cmd, Buffer buffer, u32 drawCount = 1, usz offset = 0);

void Command_Dispatch(CommandBuffer cmd, u32 groupsX, u32 groupsY = 1, u32 groupsZ = 1);

void Command_UpdateBuffer(CommandBuffer cmd, Buffer buffer, const void *data, usz size, usz offset = 0);
template <typename T>
void Command_UpdateBuffer(const CommandBuffer cmd, const Buffer buffer, const TKit::Span<const T> data, usz offset = 0)
{
    Command_UpdateBuffer(cmd, buffer, data.GetData(), data.GetBytes(), offset);
}

void Command_CopyBuffer(CommandBuffer cmd, Buffer src, Buffer dst, TKit::Span<const BufferCopy> regions);
inline void Command_CopyBuffer(const CommandBuffer cmd, const Buffer src, const Buffer dst)
{
    const BufferCopy cpy{.Size = Buffer_GetSize(src)};
    Command_CopyBuffer(cmd, src, dst, cpy);
}

void Command_CopyBufferToImage(CommandBuffer cmd, Buffer src, Image dst, TKit::Span<const BufferImageCopy> regions);
void Command_BlitImage(CommandBuffer cmd, Image src, Image dst, TKit::Span<const ImageBlit> regions,
                       Filter filter = Filter_Linear);
} // namespace Graph
