#include "pch.hpp"
#include "graph/execution.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/execution/command_pool.hpp"
#include "vkit/execution/queue.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

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
#ifdef GRAPH_HAS_PLATFORM_BACKEND
static VKit::Queue *s_PresentQueue;
#endif

void Execution_Initialize(const u32 maxPools, const u32 maxCmdBuffers)
{
    s_CommandPools.Construct();
    s_CommandBuffers.Construct();

    s_CommandPools->Reserve(maxPools);
    s_CommandBuffers->Reserve(maxCmdBuffers);

    const auto &queues = GetDevice().GetInfo().QueuesPerType;
    s_Queues[Queue_Graphics] = queues[VKit::Queue_Graphics].GetFront();
    s_Queues[Queue_Transfer] = queues[VKit::Queue_Transfer].GetFront();
    s_Queues[Queue_Compute] = queues[VKit::Queue_Compute].GetFront();
#ifdef GRAPH_HAS_PLATFORM_BACKEND
    s_PresentQueue = queues[VKit::Queue_Present].GetFront();
#endif
}

void Execution_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_CommandPools, "EXECUTION", "command pools", Pool.Destroy());

    s_Queues = {};
    s_CommandPools.Destruct();
}

Queue Queue_Get(const QueueType type)
{
    return Handle_Create(Handle_Queue, type);
}
QueueType Queue_GetType(const Queue queue)
{
    GRAPH_CHECK_HANDLE(queue, Handle_Queue);

    return QueueType(Handle_GetId(queue));
}
u64 Queue_GetCompletedTimeline(const Queue queue)
{
    GRAPH_CHECK_HANDLE(queue, Handle_Queue);

    return s_Queues[Handle_GetId(queue)]->GetCompletedTimeline();
}
u64 Queue_GetTimelineSubmissions(const Queue queue)
{
    GRAPH_CHECK_HANDLE(queue, Handle_Queue);

    return s_Queues[Handle_GetId(queue)]->GetTimelineSubmissions();
}
void Queue_SetName(const Queue queue, const char *name)
{
    GRAPH_CHECK_HANDLE(queue, Handle_Queue);

    GRAPH_CHECK_VKIT_RESULT(s_Queues[Handle_GetId(queue)]->SetName(name));
}
bool Queue_IsHandleValid(const Queue queue)
{
    if (Handle_GetType(queue) != Handle_Queue)
        return false;

    const Id id = Handle_GetId(queue);
    return id < Queue_Count && s_Queues[id];
}

CommandPool CommandPool_Create(const QueueType type, const CommandPoolFlags flags)
{
    const u32 family = GetPhysical().GetInfo().FamilyIndices[type];
    const Id id = s_CommandPools->Insert();
    Vulkan_CommandPool &pool = s_CommandPools->At(id);

    VkCommandPoolCreateFlags vkFlags = 0;
    if (flags & CommandPoolFlag_CreateTransient)
        vkFlags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    pool.Pool = GRAPH_CHECK_VKIT_RESULT(VKit::CommandPool::Create(GetDevice(), family, vkFlags));
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
void CommandPool_Reset(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);

    const Id id = Handle_GetId(pool);
    GRAPH_CHECK_VKIT_RESULT(s_CommandPools->At(id).Pool.Reset());
}
static CommandBuffer commandPool_Allocate(const CommandPool pool)
{
    const Id id = Handle_GetId(pool);
    Vulkan_CommandPool &p = s_CommandPools->At(id);

    const Id cmdId = s_CommandBuffers->Insert();
    Vulkan_CommandBuffer &cmd = s_CommandBuffers->At(cmdId);
    cmd.Pool = pool;
    cmd.Buffer = GRAPH_CHECK_VKIT_RESULT(p.Pool.Allocate());

    return Handle_Create(Handle_CommandBuffer, cmdId);
}
CommandBuffer CommandPool_NextCommandBuffer(const CommandPool pool)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);

    const Id id = Handle_GetId(pool);
    Vulkan_CommandPool &p = s_CommandPools->At(id);

    if (p.NextCommand < p.Commands.GetSize())
        return p.Commands[p.NextCommand++];

    p.NextCommand++;
    return commandPool_Allocate(pool);
}

void CommandPool_SetName(const CommandPool pool, const char *name)
{
    GRAPH_CHECK_HANDLE(pool, Handle_CommandPool);
    GRAPH_CHECK_VKIT_RESULT(s_CommandPools->At(Handle_GetId(pool)).Pool.SetName(name));
}

bool CommandPool_IsHandleValid(const CommandPool pool)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_CommandPools, pool, Handle_CommandPool);
}
bool CommandBuffer_IsHandleValid(const CommandBuffer cmd)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_CommandBuffers, cmd, Handle_CommandBuffer);
}

} // namespace Graph
