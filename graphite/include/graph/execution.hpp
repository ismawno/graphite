#pragma once

#include "graph/handle.hpp"

namespace Graph
{
enum QueueType : u8
{
    Queue_Graphics,
    Queue_Transfer,
    Queue_Compute,
    Queue_Count,
};

using CommandPoolFlags = u8;
enum CommandPoolFlagBit : u8
{
    CommandPoolFlag_CreateTransient = 1U << 0,
};

Queue Queue_Get(QueueType type);
QueueType Queue_GetType(Queue queue);
u64 Queue_GetCompletedTimeline(Queue queue);
u64 Queue_GetTimelineSubmissions(Queue queue);
void Queue_SetName(Queue queue, const char *name);
bool Queue_IsHandleValid(Queue queue);

CommandPool CommandPool_Create(QueueType type, CommandPoolFlags flags = 0);
void CommandPool_Destroy(CommandPool pool);
void CommandPool_Reset(CommandPool pool);
CommandBuffer CommandPool_NextCommandBuffer(CommandPool pool);
bool CommandPool_IsHandleValid(CommandPool pool);

bool CommandBuffer_IsHandleValid(CommandBuffer cmd);

} // namespace Graph
