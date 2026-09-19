#pragma once

#include "graph/surface.hpp"
#include "core.hpp"
#include "vkit/device/logical_device.hpp"

namespace Graph
{
VKit::Instance &GetInstance();
VKit::PhysicalDevice &GetPhysical();
VKit::LogicalDevice &GetDevice();

const VKit::Vulkan::InstanceTable *GetInstanceTable();
const VKit::Vulkan::DeviceTable *GetDeviceTable();

VkSurfaceKHR GetSurface(Surface surf);
VkSurfaceKHR CreateDummySurface();

void DestroyDummySurface();
bool IsDebugUtilsEnabled();
} // namespace Graph
