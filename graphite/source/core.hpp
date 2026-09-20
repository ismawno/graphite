#pragma once

#include "graph/core.hpp"

#define GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(stvar, handle, htype)                                                      \
    if (Handle_GetType(handle) != htype)                                                                               \
        return false;                                                                                                  \
    return stvar->Contains(Handle_GetId(handle))

#define GRAPH_CLEANUP_WITH_WARNING(stvar, section, resType, dctor)                                                     \
    TKIT_LOG_WARNING_IF(!stvar->IsEmpty(), "[GRAPH][" section "] {} " resType " have not been freed. Cleaning up...",  \
                        stvar->GetSize());                                                                             \
    for (auto &elm : *stvar)                                                                                           \
    dctor(elm)

namespace Graph
{
#ifdef GRAPH_HAS_PLATFORM_BACKEND
void Platform_Initialize(Platform plat);
void Platform_Terminate();

void Surface_Initialize(u32 maxSurfaces);
void Surface_Terminate();
#endif

void Execution_Initialize(u32 maxPools, u32 maxCmdBuffers);
void Execution_Terminate();

} // namespace Graph
