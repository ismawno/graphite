#pragma once

#include "graph/handle.hpp"

namespace Graph
{
using BufferFlags = u16;
enum BufferFlagBit : BufferFlags
{
    BufferFlag_DeviceLocal = 1U << 0,
    BufferFlag_HostVisible = 1U << 1,
    BufferFlag_Source = 1U << 2,
    BufferFlag_Destination = 1U << 3,
    BufferFlag_Staging = 1U << 4,
    BufferFlag_Vertex = 1U << 5,
    BufferFlag_Index = 1U << 6,
    BufferFlag_Storage = 1U << 7,
    BufferFlag_Indirect = 1U << 8,
    BufferFlag_HostMapped = 1U << 9,
    BufferFlag_HostRandomAccess = 1U << 10,
};

struct BufferCopy
{
    usz Size;
    usz SrcOffset = 0;
    usz DstOffset = 0;
};

Buffer Buffer_Create(usz size, BufferFlags flags);
template <typename T> Buffer Buffer_Create(const u32 count, const BufferFlags flags)
{
    return Buffer_Create(count * sizeof(T), flags);
}
void Buffer_Destroy(Buffer buffer);

usz Buffer_GetSize(Buffer buffer);
BufferFlags Buffer_GetFlags(Buffer buffer);

void *Buffer_Map(Buffer buffer);
void Buffer_Unmap(Buffer buffer);
void *Buffer_GetData();

bool Buffer_IsMapped(Buffer buffer);

void Buffer_Write(Buffer buffer, const void *data, const BufferCopy &copy);
void Buffer_Write(const Buffer buffer, const void *data, const usz size)
{
    Buffer_Write(buffer, data, {.Size = size});
}

void Buffer_Flush();

bool Buffer_IsHandleValid(Buffer buffer);

} // namespace Graph
