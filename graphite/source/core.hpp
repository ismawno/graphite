#pragma once

#include "graph/core.hpp"

#define GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(stvar, handle, htype)                                                      \
    if (Handle_GetType(handle) != htype)                                                                               \
        return false;                                                                                                  \
    return stvar->Contains(Handle_GetId(handle))

#define GRAPH_CLEANUP_WITH_WARNING_LAMBDA(stvar, section, resType, dctor)                                              \
    TKIT_LOG_WARNING_IF(!stvar->IsEmpty(), "[GRAPH][" section "] {} " resType " have not been freed. Cleaning up...",  \
                        stvar->GetSize());                                                                             \
    for (auto &elm : *stvar)                                                                                           \
    dctor(elm)
#define GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(stvar, section, resType, accessor)                                         \
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(stvar, section, resType, [](auto &elm) { elm.accessor; })

namespace Graph
{
#ifdef GRAPH_HAS_PLATFORM_BACKEND
void Platform_Initialize(Platform plat, u32 maxWindows);
void Platform_Terminate();

void Surface_Initialize(u32 maxSurfaces);
void Surface_Terminate();
#endif

void Execution_Initialize(u32 maxPools, u32 maxCmdBuffers);
void Execution_Terminate();

void Resources_Initialize(u32 maxBuffers, u32 maxImages, u32 maxSamplers);
void Resources_Terminate();

} // namespace Graph
