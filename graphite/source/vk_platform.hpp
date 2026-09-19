#pragma once

#include "graph/core.hpp"
#include <vulkan/vulkan.h>

namespace Graph
{
VkSurfaceKHR Platform_CreateSurface(Window win);
void Platform_DestroySurface(VkSurfaceKHR surf);
} // namespace Graph
