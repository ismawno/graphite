#pragma once

#include "graph/core.hpp"
#include "core.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/memory/allocator.hpp"
#include "vkit/resource/device_buffer.hpp"
#include "vkit/resource/device_image.hpp"
#include "vkit/resource/sampler.hpp"
#include "vkit/state/descriptor_set.hpp"
#include "vkit/state/pipeline_layout.hpp"
#include "vkit/state/shader.hpp"

namespace Graph
{
struct ImageSubresourceRange;
struct ImageSubresourceLayers;
struct Tracker;

VKit::Instance &GetInstance();
VKit::PhysicalDevice &GetPhysical();
VKit::LogicalDevice &GetDevice();

const VKit::Vulkan::InstanceTable *GetInstanceTable();
const VKit::Vulkan::DeviceTable *GetDeviceTable();

VmaAllocator GetAllocator();

VKit::DeviceBuffer &GetBuffer(Buffer buffer);
VKit::DeviceImage &GetImage(Image image);
VkImageView GetImageView(ImageView view);
VKit::Sampler &GetSampler(Sampler sampler);
VKit::DescriptorSetLayout &GetDescriptorSetLayout(DescriptorSetLayout layout);
VKit::DescriptorSet &GetDescriptorSet(DescriptorSet set);
VKit::Shader &GetShader(Shader sh);
VKit::PipelineLayout &GetPipelineLayout(PipelineLayout layout);
// useful for swap chains, which need to expose handles but destroys its own images. Image_Destroy is still needed to
// unregister it from resources!!
Image CreateNonOwnedImage(const VKit::DeviceImage &img);

#ifdef GRAPH_HAS_PLATFORM_BACKEND
VkSurfaceKHR GetSurface(Window win);
VkSemaphore GetRenderFinishedSemaphore(Swapchain sc);
VkSemaphore GetImageAvailableSemaphore(Swapchain sc);
void SetRenderTimelineTracker(Swapchain sc, const Tracker &tracker);
#endif

void BindPipeline(VkCommandBuffer cmd, Pipeline pip);

bool IsDebugUtilsEnabled();

VkFormat ToVulkan(Format format);
VkImageTiling ToVulkan(ImageTiling tiling);
VkImageLayout ToVulkan(ImageLayout layout);
VkSampleCountFlagBits ToVulkan(SampleCount samples);
VkImageType ToVulkan(ImageType type);
VkImageViewType ToVulkan(ImageViewType type);
VkImageAspectFlags ToVulkanImageAspectFlags(ImageAspectFlags aspects);
VkImageAspectFlags ToVulkanImageAspectFlags(const VKit::DeviceImage &img, ImageAspectFlags aspects);
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
VkImageSubresourceRange ToVulkan(const VKit::DeviceImage &img, const ImageSubresourceRange &range);
VkImageSubresourceLayers ToVulkan(const VKit::DeviceImage &img, const ImageSubresourceLayers &layers);
VkDescriptorType ToVulkan(DescriptorType type);
VkAttachmentLoadOp ToVulkan(LoadOp op);
VkAttachmentStoreOp ToVulkan(StoreOp op);
VkResolveModeFlagBits ToVulkan(ResolveMode mode);
VkPipelineStageFlags2KHR ToVulkanPipelineStageFlags(PipelineStageFlags stages);
VkAccessFlags2KHR ToVulkanAccessFlags(AccessFlags access);
VkPipelineBindPoint ToVulkan(BindPoint point);
VkIndexType ToVulkan(IndexType type);
#ifdef GRAPH_HAS_PLATFORM_BACKEND
VkPresentModeKHR ToVulkan(PresentMode mode);
#endif
VKit::DeviceBufferFlags ToVulkanBufferFlags(BufferFlags flags);
VKit::DeviceImageFlags ToVulkanImageFlags(ImageFlags flags);
VKit::QueueType ToVulkan(QueueType type);
} // namespace Graph
