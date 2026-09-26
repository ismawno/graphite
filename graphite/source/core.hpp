#pragma once

#include "graph/core.hpp"

#define GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(stvar, handle, htype)                                                      \
    if (Handle_GetType(handle) != htype)                                                                               \
        return false;                                                                                                  \
    return stvar->Contains(Handle_GetId(handle))

#define GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(stvar, handle, dctor)                                                       \
    const Id id = Handle_GetId(handle);                                                                                \
    dctor(stvar->At(id));                                                                                              \
    stvar->Remove(id)

#define GRAPH_DESTROY_FUNCTION_BODY_ACCESSOR(stvar, handle, accessor)                                                  \
    GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(stvar, handle, [](auto &elm) { elm.accessor; })

#define GRAPH_DESTROY_FUNCTION_BODY(stvar, handle) GRAPH_DESTROY_FUNCTION_BODY_ACCESSOR(stvar, handle, Destroy())

#define GRAPH_CLEANUP_WITH_WARNING_LAMBDA(stvar, section, resType, dctor)                                              \
    TKIT_LOG_WARNING_IF(!stvar->IsEmpty(), "[GRAPH][" section "] {} " resType " have not been freed. Cleaning up...",  \
                        stvar->GetSize());                                                                             \
    for (auto &elm : *stvar)                                                                                           \
    dctor(elm)

#define GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(stvar, section, resType, accessor)                                         \
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(stvar, section, resType, [](auto &elm) { elm.accessor; })
#define GRAPH_CLEANUP_WITH_WARNING(stvar, section, resType)                                                            \
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(stvar, section, resType, Destroy())

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

void Resources_Initialize(u32 maxBuffers, u32 maxImages, u32 maxSamplers, u32 maxViews);
void Resources_Terminate();

void Shader_Initialize(u32 maxShaders);
void Shader_Terminate();

#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
void Compilation_Initialize(u32 maxCompilations);
void Compilation_Terminate();
#endif

} // namespace Graph
