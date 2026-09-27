#pragma once

#include "graph/core.hpp"
#include "core.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/resource/device_image.hpp"
#include "vkit/resource/sampler.hpp"
#include "vkit/state/descriptor_set_layout.hpp"

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
VkImageView GetImageView(ImageView view);
VKit::Sampler &GetSampler(Sampler sampler);
VKit::DescriptorSetLayout &GetDescriptorSetLayout(DescriptorSetLayout layout);

void DestroyDummySurface();
bool IsDebugUtilsEnabled();

VkFormat ToVulkan(Format format);
VkImageTiling ToVulkan(ImageTiling tiling);
VkImageLayout ToVulkan(ImageLayout layout);
VkSampleCountFlagBits ToVulkan(SampleCount samples);
VkImageType ToVulkan(ImageType type);
VkImageViewType ToVulkan(ImageViewType type);
VkImageAspectFlags ToVulkanImageAspectFlags(ImageAspectFlags aspects);
VkSamplerMipmapMode ToVulkan(SamplerMode mode);
VkFilter ToVulkan(Filter filter);
VkSamplerAddressMode ToVulkan(Wrap wrap);
VkCompareOp ToVulkan(CompareOp op);
VkBorderColor ToVulkan(BorderColor color);
VkPrimitiveTopology ToVulkan(Topology topology);
VkPolygonMode ToVulkan(PolygonMode mode);
VkCullModeFlags ToVulkan(CullMode mode);
VkFrontFace ToVulkan(FrontFace face);
VkBlendFactor ToVulkan(BlendFactor factor);
VkBlendOp ToVulkan(BlendOp op);
VkColorComponentFlags ToVulkanColorWriteMask(ColorWriteMask mask);
VkStencilOp ToVulkan(StencilOp op);
VkVertexInputRate ToVulkan(VertexInputRate rate);
VkShaderStageFlags ToVulkanShaderStageFlags(ShaderStageFlags stages);
VkDescriptorBindingFlags ToVulkanDescriptorBindingFlags(DescriptorBindingFlags flags);
VkDescriptorType ToVulkan(DescriptorType type);
} // namespace Graph
