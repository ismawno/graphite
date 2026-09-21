#pragma once

#ifndef GRAPH_HAS_PLATFORM_BACKEND
#    error                                                                                                             \
        "[GRAPH][PLATFORM] To use surface capabilities, a platform backend must be specified with the CMake option GRAPHITE_PLATFORM_BACKEND"
#endif

#include "graph/handle.hpp"
#include "graph/core.hpp"

namespace Graph
{
Surface Surface_Create(Window win);
void Surface_Destroy(Surface surf);

bool Surface_IsHandleValid(Surface surf);
} // namespace Graph
