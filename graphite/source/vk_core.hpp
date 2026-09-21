#pragma once

#include "graph/handle.hpp"
#include "core.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/resource/device_image.hpp"
#include "vkit/resource/sampler.hpp"

namespace Graph
{
VKit::Instance &GetInstance();
VKit::PhysicalDevice &GetPhysical();
VKit::LogicalDevice &GetDevice();

const VKit::Vulkan::InstanceTable *GetInstanceTable();
const VKit::Vulkan::DeviceTable *GetDeviceTable();

VkSurfaceKHR GetSurface(Surface surf);
VkSurfaceKHR CreateDummySurface();
VmaAllocator GetAllocator();

VKit::DeviceBuffer &GetBuffer(Buffer buffer);
VKit::DeviceImage &GetImage(Image image);
VKit::Sampler &GetSampler(Sampler sampler);

void DestroyDummySurface();
bool IsDebugUtilsEnabled();
} // namespace Graph
