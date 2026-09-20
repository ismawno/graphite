#pragma once

#ifndef GRAPH_HAS_PLATFORM_BACKEND
#    error                                                                                                             \
        "[GRAPH][PLATFORM] To use platform capabilities, a platform backend must be specified with the CMake option GRAPHITE_PLATFORM_BACKEND"
#endif

#include "graph/core.hpp"
#include <vulkan/vulkan.h>

namespace Graph
{
VkSurfaceKHR Platform_CreateSurface(Window win);
void Platform_DestroySurface(VkSurfaceKHR surf);
} // namespace Graph
