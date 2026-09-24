#include "pch.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "graph/resources.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
struct Vulkan_ImageView
{
    Image Image;
    VkImageView View;
};

static TKit::Storage<TKit::ArenaHive<VKit::DeviceBuffer>> s_Buffers{};
static TKit::Storage<TKit::ArenaHive<VKit::DeviceImage>> s_Images{};
static TKit::Storage<TKit::ArenaHive<VKit::Sampler>> s_Samplers{};
static TKit::Storage<TKit::ArenaHive<Vulkan_ImageView>> s_Views{};

void Resources_Initialize(const u32 maxBuffers, const u32 maxImages, const u32 maxSamplers, const u32 maxViews)
{
    s_Buffers.Construct();
    s_Images.Construct();
    s_Samplers.Construct();
    s_Views.Construct();

    s_Buffers->Reserve(maxBuffers);
    s_Images->Reserve(maxImages);
    s_Samplers->Reserve(maxSamplers);
    s_Views->Reserve(maxViews);
}

void Resources_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_Buffers, "RESOURCES", "buffers", Destroy());
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_Images, "RESOURCES", "images", Destroy());
    GRAPH_CLEANUP_WITH_WARNING_ACCESSOR(s_Samplers, "RESOURCES", "samplers", Destroy());

    TKIT_ASSERT(s_Views->IsEmpty(),
                "[GRAPH][RESOURCES] Views should have been deleted along with images, but there are {} remaining",
                s_Views->GetSize());

    s_Views.Destruct();
    s_Buffers.Destruct();
    s_Images.Destruct();
    s_Samplers.Destruct();
}

Buffer Buffer_Create(const usz size, const BufferFlags flags)
{
    const VKit::DeviceBuffer buff = GRAPH_CHECK_VKIT_RESULT(
        VKit::DeviceBuffer::Builder(GetDevice(), GetAllocator(), VKit::DeviceBufferFlags(flags)).SetSize(size).Build());

    return Handle_Create(Handle_Buffer, s_Buffers->Insert(buff));
}
void Buffer_Destroy(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    GRAPH_DESTROY_FUNCTION_BODY(s_Buffers, buffer);
}

usz Buffer_GetSize(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    return s_Buffers->At(Handle_GetId(buffer)).GetInfo().Size;
}
BufferFlags Buffer_GetFlags(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    return BufferFlags(s_Buffers->At(Handle_GetId(buffer)).GetInfo().Flags);
}

void *Buffer_Map(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    VKit::DeviceBuffer &buff = s_Buffers->At(Handle_GetId(buffer));
    GRAPH_CHECK_VKIT_RESULT(buff.Map());
    return buff.GetData();
}
void Buffer_Unmap(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    s_Buffers->At(Handle_GetId(buffer)).Unmap();
}
bool Buffer_IsMap(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    return s_Buffers->At(Handle_GetId(buffer)).IsMapped();
}

void Buffer_Write(const Buffer buffer, const void *data, const BufferCopy &copy)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    s_Buffers->At(Handle_GetId(buffer))
        .Write(data, {.srcOffset = copy.SrcOffset, .dstOffset = copy.DstOffset, .size = copy.Size});
}
void Buffer_Flush(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    GRAPH_CHECK_VKIT_RESULT(s_Buffers->At(Handle_GetId(buffer)).Flush());
}
void Buffer_SetName(const Buffer buffer, const char *name)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    GRAPH_CHECK_VKIT_RESULT(s_Buffers->At(Handle_GetId(buffer)).SetName(name));
}

bool Buffer_IsHandleValid(const Buffer buffer)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Buffers, buffer, Handle_Buffer);
}

static VkFormat toVulkan(const ImageFormat format)
{
    switch (format)
    {
    case ImageFormat_Undefined:
        return VK_FORMAT_UNDEFINED;

    case ImageFormat_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case ImageFormat_R8_SNORM:
        return VK_FORMAT_R8_SNORM;
    case ImageFormat_R8_UINT:
        return VK_FORMAT_R8_UINT;
    case ImageFormat_R8_SINT:
        return VK_FORMAT_R8_SINT;
    case ImageFormat_R8_SRGB:
        return VK_FORMAT_R8_SRGB;

    case ImageFormat_R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case ImageFormat_R8G8_SNORM:
        return VK_FORMAT_R8G8_SNORM;
    case ImageFormat_R8G8_UINT:
        return VK_FORMAT_R8G8_UINT;
    case ImageFormat_R8G8_SINT:
        return VK_FORMAT_R8G8_SINT;
    case ImageFormat_R8G8_SRGB:
        return VK_FORMAT_R8G8_SRGB;

    case ImageFormat_R8G8B8_UNORM:
        return VK_FORMAT_R8G8B8_UNORM;
    case ImageFormat_R8G8B8_SNORM:
        return VK_FORMAT_R8G8B8_SNORM;
    case ImageFormat_R8G8B8_UINT:
        return VK_FORMAT_R8G8B8_UINT;
    case ImageFormat_R8G8B8_SINT:
        return VK_FORMAT_R8G8B8_SINT;
    case ImageFormat_R8G8B8_SRGB:
        return VK_FORMAT_R8G8B8_SRGB;

    case ImageFormat_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat_R8G8B8A8_SNORM:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case ImageFormat_R8G8B8A8_UINT:
        return VK_FORMAT_R8G8B8A8_UINT;
    case ImageFormat_R8G8B8A8_SINT:
        return VK_FORMAT_R8G8B8A8_SINT;
    case ImageFormat_R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case ImageFormat_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case ImageFormat_B8G8R8A8_SNORM:
        return VK_FORMAT_B8G8R8A8_SNORM;
    case ImageFormat_B8G8R8A8_UINT:
        return VK_FORMAT_B8G8R8A8_UINT;
    case ImageFormat_B8G8R8A8_SINT:
        return VK_FORMAT_B8G8R8A8_SINT;
    case ImageFormat_B8G8R8A8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;

    case ImageFormat_R16_UNORM:
        return VK_FORMAT_R16_UNORM;
    case ImageFormat_R16_SNORM:
        return VK_FORMAT_R16_SNORM;
    case ImageFormat_R16_UINT:
        return VK_FORMAT_R16_UINT;
    case ImageFormat_R16_SINT:
        return VK_FORMAT_R16_SINT;
    case ImageFormat_R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;

    case ImageFormat_R16G16_UNORM:
        return VK_FORMAT_R16G16_UNORM;
    case ImageFormat_R16G16_SNORM:
        return VK_FORMAT_R16G16_SNORM;
    case ImageFormat_R16G16_UINT:
        return VK_FORMAT_R16G16_UINT;
    case ImageFormat_R16G16_SINT:
        return VK_FORMAT_R16G16_SINT;
    case ImageFormat_R16G16_SFLOAT:
        return VK_FORMAT_R16G16_SFLOAT;

    case ImageFormat_R16G16B16_UNORM:
        return VK_FORMAT_R16G16B16_UNORM;
    case ImageFormat_R16G16B16_SNORM:
        return VK_FORMAT_R16G16B16_SNORM;
    case ImageFormat_R16G16B16_UINT:
        return VK_FORMAT_R16G16B16_UINT;
    case ImageFormat_R16G16B16_SINT:
        return VK_FORMAT_R16G16B16_SINT;
    case ImageFormat_R16G16B16_SFLOAT:
        return VK_FORMAT_R16G16B16_SFLOAT;

    case ImageFormat_R16G16B16A16_UNORM:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case ImageFormat_R16G16B16A16_SNORM:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case ImageFormat_R16G16B16A16_UINT:
        return VK_FORMAT_R16G16B16A16_UINT;
    case ImageFormat_R16G16B16A16_SINT:
        return VK_FORMAT_R16G16B16A16_SINT;
    case ImageFormat_R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;

    case ImageFormat_R32_UINT:
        return VK_FORMAT_R32_UINT;
    case ImageFormat_R32_SINT:
        return VK_FORMAT_R32_SINT;
    case ImageFormat_R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;

    case ImageFormat_R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case ImageFormat_R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case ImageFormat_R32G32_SFLOAT:
        return VK_FORMAT_R32G32_SFLOAT;

    case ImageFormat_R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case ImageFormat_R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case ImageFormat_R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;

    case ImageFormat_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case ImageFormat_R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case ImageFormat_R32G32B32A32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

    case ImageFormat_D16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case ImageFormat_D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;

    case ImageFormat_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case ImageFormat_D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;

    case ImageFormat_BC1_RGBA_UNORM:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case ImageFormat_BC1_RGBA_SRGB:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case ImageFormat_BC5_UNORM:
        return VK_FORMAT_BC5_UNORM_BLOCK;
    case ImageFormat_BC5_SNORM:
        return VK_FORMAT_BC5_SNORM_BLOCK;
    case ImageFormat_BC7_UNORM:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case ImageFormat_BC7_SRGB:
        return VK_FORMAT_BC7_SRGB_BLOCK;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static VkImageTiling toVulkan(const ImageTiling tiling)
{
    switch (tiling)
    {
    case ImageTiling_Optimal:
        return VK_IMAGE_TILING_OPTIMAL;
    case ImageTiling_Linear:
        return VK_IMAGE_TILING_LINEAR;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown image tiling: {}", u32(tiling));
        return VK_IMAGE_TILING_OPTIMAL;
    }
}

static VkImageLayout toVulkan(const ImageLayout layout)
{
    switch (layout)
    {
    case ImageLayout_Undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case ImageLayout_General:
        return VK_IMAGE_LAYOUT_GENERAL;
    case ImageLayout_ColorAttachment:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout_DepthStencilAttachment:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ImageLayout_DepthStencilReadOnly:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ImageLayout_ShaderReadOnly:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout_TransferSrc:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout_TransferDst:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout_Present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown image layout: {}", u32(layout));
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

static VkSampleCountFlagBits toVulkan(const SampleCount samples)
{
    switch (samples)
    {
    case SampleCount_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case SampleCount_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case SampleCount_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case SampleCount_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case SampleCount_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case SampleCount_32:
        return VK_SAMPLE_COUNT_32_BIT;
    case SampleCount_64:
        return VK_SAMPLE_COUNT_64_BIT;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown sample count: {}", u32(samples));
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

static VkImageType toVulkan(const ImageType type)
{
    switch (type)
    {
    case ImageType_1D:
        return VK_IMAGE_TYPE_1D;
    case ImageType_2D:
        return VK_IMAGE_TYPE_2D;
    case ImageType_3D:
        return VK_IMAGE_TYPE_3D;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown image type: {}", u32(type));
        return VK_IMAGE_TYPE_2D;
    }
}

static VkImageViewType toVulkan(const ImageViewType type)
{
    switch (type)
    {
    case ImageViewType_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case ImageViewType_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case ImageViewType_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown image view type: {}", u32(type));
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

static VkImageAspectFlags toVulkan(const ImageAspectFlags aspects)
{
    VkImageAspectFlags result = 0;
    if (aspects & ImageAspectFlag_Color)
        result |= VK_IMAGE_ASPECT_COLOR_BIT;
    if (aspects & ImageAspectFlag_Depth)
        result |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (aspects & ImageAspectFlag_Stencil)
        result |= VK_IMAGE_ASPECT_STENCIL_BIT;
    return result;
}

static VkSamplerMipmapMode toVulkan(const SamplerMode mode)
{
    switch (mode)
    {
    case SamplerMode_Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case SamplerMode_Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown sampler mode: {}", u32(mode));
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

static VkFilter toVulkan(const Filter filter)
{
    switch (filter)
    {
    case Filter_Linear:
        return VK_FILTER_LINEAR;
    case Filter_Nearest:
        return VK_FILTER_NEAREST;
    case Filter_Cubic:
        return VK_FILTER_CUBIC_EXT;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown filter: {}", u32(filter));
        return VK_FILTER_LINEAR;
    }
}

static VkSamplerAddressMode toVulkan(const Wrap wrap)
{
    switch (wrap)
    {
    case Wrap_Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case Wrap_ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case Wrap_MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown wrap mode: {}", u32(wrap));
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

static VkCompareOp toVulkan(const CompareOperation op)
{
    switch (op)
    {
    case Compare_Never:
        return VK_COMPARE_OP_NEVER;
    case Compare_Less:
        return VK_COMPARE_OP_LESS;
    case Compare_Equal:
        return VK_COMPARE_OP_EQUAL;
    case Compare_LessOrEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case Compare_Greater:
        return VK_COMPARE_OP_GREATER;
    case Compare_NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case Compare_GreaterOrEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case Compare_Always:
        return VK_COMPARE_OP_ALWAYS;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown compare operation: {}", u32(op));
        return VK_COMPARE_OP_NEVER;
    }
}

static VkBorderColor toVulkan(const BorderColor color)
{
    switch (color)
    {
    case BorderColor_FloatTransparentBlack:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case BorderColor_IntTransparentBlack:
        return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case BorderColor_FloatOpaqueBlack:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case BorderColor_IntOpaqueBlack:
        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    case BorderColor_FloatOpaqueWhite:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case BorderColor_IntOpaqueWhite:
        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown border color: {}", u32(color));
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }
}

Image Image_Create(const u32v3 &size, const ImageSpecs &specs, const ImageFlags flags)
{
    TKit::StackArray<VkFormat> formats{};
    for (const ImageFormat fmt : specs.Formats)
        formats.Append(toVulkan(fmt));

    VKit::DeviceImage::Builder builder{
        GetDevice(), GetAllocator(), {size[0], size[1], size[2]}, formats, VKit::DeviceImageFlags(flags)};
    builder.SetMipLevels(specs.MipLevels)
        .SetArrayLayers(specs.ArrayLayers)
        .SetImageType(toVulkan(specs.Type))
        .SetTiling(toVulkan(specs.Tiling))
        .SetInitialLayout(toVulkan(specs.InitialLayout))
        .SetSamples(toVulkan(specs.Samples));
    if (flags & ImageFlag_CubeCompatible)
        builder.SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

    const VKit::DeviceImage img = GRAPH_CHECK_VKIT_RESULT(builder.Build());

    const Image handle = Handle_Create(Handle_Image, s_Images->Insert(img));
    for (const ImageViewSpecs &vspc : specs.ImageViews)
        Image_AddView(handle, vspc);

    return handle;
}

void Image_Destroy(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    const TKit::StackArray<Id> viewIds = s_Views->GetValidIds();
    for (const Id id : viewIds)
        if (s_Views->At(id).Image == img)
            s_Views->Remove(id);

    GRAPH_DESTROY_FUNCTION_BODY(s_Images, img);
}
void Image_DestroyViews(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    s_Images->At(Handle_GetId(img)).DestroyImageViews();
}

usz Image_ComputeSize(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    return s_Images->At(Handle_GetId(img)).ComputeSize();
}

ImageView Image_AddView(const Image img, const ImageViewSpecs &specs)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    VKit::DeviceImage &image = s_Images->At(Handle_GetId(img));

    VkImageViewCreateInfo info{};
    info.image = VK_NULL_HANDLE;
    info.viewType = toVulkan(specs.Type);
    info.subresourceRange.aspectMask = toVulkan(specs.Range.Aspect);
    info.subresourceRange.baseMipLevel = specs.Range.MipStart;
    info.subresourceRange.levelCount = specs.Range.MipCount;
    info.subresourceRange.baseArrayLayer = specs.Range.LayerStart;
    info.subresourceRange.layerCount = specs.Range.LayerCount;
    info.format = toVulkan(specs.Format);
    const VkImageView view = GRAPH_CHECK_VKIT_RESULT(image.AddImageView(info));

    return Handle_Create(Handle_ImageView, s_Views->Insert(img, view));
}

ImageView Image_GetView(const Image img, const u32 idx)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);

    VKit::DeviceImage &image = s_Images->At(Handle_GetId(img));
    const VkImageView view = image.GetView(idx);
    for (const Id id : s_Views->GetValidIds())
        if (s_Views->At(id).View == view)
            return Handle_Create(Handle_ImageView, id);

    TKIT_FATAL("[GRAPH][RESOURCES] Could not find the associated image view");
    return NullHandle;
}

void Image_SetLayout(const Image img, const ImageLayout layout)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    s_Images->At(Handle_GetId(img)).SetLayout(toVulkan(layout));
}

void Image_SetName(const Image img, const char *name)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    GRAPH_CHECK_VKIT_RESULT(s_Images->At(Handle_GetId(img)).SetName(name));
}
void Image_SetViewName(const Image img, const char *name)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    GRAPH_CHECK_VKIT_RESULT(s_Images->At(Handle_GetId(img)).SetViewNames(name));
}
bool Image_IsHandleValid(const Image img)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Images, img, Handle_Image);
}

void ImageView_SetName(const ImageView view, const char *name)
{
    GRAPH_CHECK_HANDLE(view, Handle_ImageView);
    GRAPH_CHECK_VKIT_RESULT(
        GetDevice().SetObjectName(s_Views->At(Handle_GetId(view)).View, VK_OBJECT_TYPE_IMAGE_VIEW, name));
}
bool ImageView_IsHandleValid(const ImageView view)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Views, view, Handle_ImageView);
}

Sampler Sampler_Create(const SamplerSpecs &specs, const SamplerFlags flags)
{
    const VKit::Sampler smp = GRAPH_CHECK_VKIT_RESULT(
        VKit::Sampler::Builder(GetDevice())
            .SetMipmapMode(toVulkan(specs.Mode))
            .SetFilters(toVulkan(specs.Filters[0]), toVulkan(specs.Filters[1]))
            .SetAddressModes(toVulkan(specs.Wraps[0]), toVulkan(specs.Wraps[0]), toVulkan(specs.Wraps[0]))
            .SetLodRange(specs.LodRange[0], specs.LodRange[1])
            .SetAnisotropy(specs.MaxAnisotropy)
            .SetCompareOp(toVulkan(specs.Compare))
            .SetBorderColor(toVulkan(specs.Border))
            .EnableAnisotropy((flags & SamplerFlag_EnableAnisotropy) != 0)
            .EnableCompare(specs.Compare != Compare_Always)
            .SetUnnormalizedCoordinates((flags & SamplerFlag_EnableUnnormalizedCoordinates) != 0)
            .Build());

    return Handle_Create(Handle_Sampler, s_Samplers->Insert(smp));
}

void Sampler_Destroy(const Sampler smp)
{
    GRAPH_CHECK_HANDLE(smp, Handle_Sampler);
    GRAPH_DESTROY_FUNCTION_BODY(s_Samplers, smp);
}

void Sampler_SetName(const Sampler smp, const char *name)
{
    GRAPH_CHECK_HANDLE(smp, Handle_Sampler);
    GRAPH_CHECK_VKIT_RESULT(s_Samplers->At(Handle_GetId(smp)).SetName(name));
}

bool Sampler_IsHandleValid(const Sampler smp)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Samplers, smp, Handle_Sampler);
}

VKit::DeviceBuffer &GetBuffer(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    return s_Buffers->At(Handle_GetId(buffer));
}
VKit::DeviceImage &GetImage(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    return s_Images->At(Handle_GetId(img));
}
VKit::Sampler &GetSampler(const Sampler smp)
{
    GRAPH_CHECK_HANDLE(smp, Handle_Sampler);
    return s_Samplers->At(Handle_GetId(smp));
}
} // namespace Graph
