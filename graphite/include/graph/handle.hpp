#pragma once

#include "graph/alias.hpp"
#include "tkit/utils/limits.hpp"
#include "tkit/utils/debug.hpp"

#define GRAPH_HANDLE_WIDTH 32U
#define GRAPH_HANDLE_MASK (~0U)

///

#define GRAPH_HANDLE_TYPE_BITS 4U

#define GRAPH_HANDLE_TYPE_SHIFT (GRAPH_HANDLE_WIDTH - GRAPH_HANDLE_TYPE_BITS)
#define GRAPH_HANDLE_ID_BITS GRAPH_HANDLE_TYPE_SHIFT

#define GRAPH_HANDLE_TYPE_MASK (GRAPH_HANDLE_MASK << GRAPH_HANDLE_TYPE_SHIFT)
#define GRAPH_HANDLE_ID_MASK ~GRAPH_HANDLE_TYPE_MASK

#define GRAPH_NULL_HANDLE_ID GRAPH_HANDLE_ID_MASK

#define GRAPH_MAX_HANDLE_TYPES ((1U << GRAPH_HANDLE_TYPE_BITS) - 1U)
#define GRAPH_MAX_HANDLE_IDS ((1U << GRAPH_HANDLE_ID_BITS) - 1U)

#ifdef TKIT_ENABLE_ENSURE
#    define GRAPH_CHECK_HANDLE_HAS_VALID_TYPE(hndl)                                                                    \
        TKIT_ASSERT(Graph::Handle_GetTypeAsInteger(hndl) < Graph::Handle_Count,                                        \
                    "[GRAPH][HANDLE] The handle {:#010x} does not match any known handle types ({}), which likely "    \
                    "means it is a "                                                                                   \
                    "broken handle",                                                                                   \
                    hndl, Graph::Handle_GetTypeAsInteger(hndl))

#    define __GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype)                                                                 \
        TKIT_ASSERT(Graph::Handle_GetType(hndl) == htype,                                                              \
                    "[GRAPH][HANDLE] The handle {:#010x} is not a '{}' handle, but rather a '{}' handle", hndl,        \
                    Graph::ToString(htype), Graph::ToString(Graph::Handle_GetType(hndl)))

#    define GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype)                                                                   \
        GRAPH_CHECK_HANDLE_HAS_VALID_TYPE(hndl);                                                                       \
        __GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype)

#    define GRAPH_CHECK_ID_IS_NOT_NULL(hndl)                                                                           \
        TKIT_ASSERT(!Graph::Handle_IsIdNull(hndl), "[GRAPH][HANDLE] The handle {:#010x} has a null id", hndl)

#    define GRAPH_CHECK_HANDLE_IS_VALID(hndl, htype)                                                                   \
        GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype);                                                                      \
        TKIT_ASSERT(Graph::Handle_IsValid(hndl, htype),                                                                \
                    "[GRAPH][HANDLE] The handle {:#010x} is not a valid '{}' handle", hndl, ToString(htype))

#    define GRAPH_CHECK_HANDLE(hndl, htype)                                                                            \
        GRAPH_CHECK_ID_IS_NOT_NULL(hndl);                                                                              \
        GRAPH_CHECK_HANDLE_IS_VALID(hndl, htype)

#else
#    define GRAPH_CHECK_HANDLE_HAS_VALID_TYPE(...)
#    define GRAPH_CHECK_HANDLE_HAS_VALID_RESOURCE_POOL_TYPE(...)
#    define GRAPH_CHECK_HANDLE_HAS_TYPE(...)
#    define GRAPH_CHECK_HANDLE_HAS_RESOURCE_POOL_TYPE(...)
#    define GRAPH_CHECK_RESOURCE_IS_NOT_NULL(...)
#    define GRAPH_CHECK_RESOURCE_POOL_IS_NOT_NULL(...)
#    define GRAPH_CHECK_RESOURCE_POOL_ID_IS_NOT_NULL(...)
#    define GRAPH_CHECK_RESOURCE_IS_VALID(...)
#    define GRAPH_CHECK_RESOURCE_IS_VALID_WITH_DIM(...)
#    define GRAPH_CHECK_RESOURCE_POOL_IS_VALID(...)
#    define GRAPH_CHECK_RESOURCE_POOL_IS_VALID_WITH_DIM(...)
#endif

namespace Graph
{
using Handle = u<GRAPH_HANDLE_WIDTH>;
constexpr Handle NullHandle = TKit::Limits<Handle>::Max();

using Id = Handle;
constexpr Id NullId = GRAPH_NULL_HANDLE_ID;

using Window = Handle;
using Monitor = Handle;
using Surface = Handle;
using Queue = Handle;
using CommandPool = Handle;
using CommandBuffer = Handle;
using Buffer = Handle;
using Image = Handle;
using Sampler = Handle;

enum HandleType : u8
{
    Handle_Window,
    Handle_Monitor,
    Handle_Surface,
    Handle_Queue,
    Handle_CommandPool,
    Handle_CommandBuffer,
    Handle_Buffer,
    Handle_Image,
    Handle_Sampler,
    Handle_Count,
    Handle_None = Handle_Count
};

static_assert(Handle_Count <= GRAPH_MAX_HANDLE_TYPES,
              "[GRAPH][HANDLE] The handle type count exceeds maximum handle types");

const char *ToString(HandleType htype);

constexpr u32 Handle_GetTypeAsInteger(const Handle handle)
{
    return (handle & GRAPH_HANDLE_TYPE_MASK) >> GRAPH_HANDLE_TYPE_SHIFT;
}

constexpr HandleType Handle_GetType(const Handle handle)
{
    return HandleType(Handle_GetTypeAsInteger(handle));
}

constexpr bool Handle_IsIdNull(const Handle handle)
{
    return (handle & GRAPH_HANDLE_ID_MASK) == NullId;
}

constexpr Id Handle_GetId(const Handle handle)
{
    return handle & GRAPH_HANDLE_ID_MASK;
}

constexpr Handle Handle_Create(const HandleType htype, const Id id)
{
    TKIT_ASSERT(htype < Handle_Count, "[GRAPH][HANDLE] Cannot create a handle with an invalid type");
    TKIT_ASSERT(id <= GRAPH_MAX_HANDLE_IDS,
                "[GRAPH][HANDLE] Cannot create a handle with an id ({:#010x}) "
                "that exceeds the maximum bits allocated "
                "for it, as it would break the handle",
                id);
    return (u32(htype) << GRAPH_HANDLE_TYPE_SHIFT) | id;
}

bool Handle_IsValid(Handle handle, HandleType htype = Handle_None);
} // namespace Graph
