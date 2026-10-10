#include "pch.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "graph/resources.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
struct Vulkan_Image
{
    VKit::DeviceImage Image{};
    bool MustDestroy = true;
};
struct Vulkan_ImageView
{
    Image Image;
    VkImageView View;
};

static TKit::Storage<TKit::ArenaHive<VKit::DeviceBuffer>> s_Buffers{};
static TKit::Storage<TKit::ArenaHive<Vulkan_Image>> s_Images{};
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

static void image_Destroy(Vulkan_Image &img)
{
    if (img.MustDestroy)
        img.Image.Destroy();
}

void Resources_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING(s_Buffers, "RESOURCES", "buffers");
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(s_Images, "RESOURCES", "images", image_Destroy);
    GRAPH_CLEANUP_WITH_WARNING(s_Samplers, "RESOURCES", "samplers");

    TKIT_ASSERT(s_Views->IsEmpty(),
                "[GRAPH][RESOURCES] Views should have been deleted along with images, but there are {} remaining",
                s_Views->GetSize());

    s_Views.Destruct();
    s_Buffers.Destruct();
    s_Images.Destruct();
    s_Samplers.Destruct();
}

usz Format_GetBytesPerPixel(const Format fmt)
{
    return VKit::DeviceImage::GetBytesPerPixel(ToVulkan(fmt));
}

Buffer Buffer_Create(const usz size, const BufferFlags flags)
{
    const VKit::DeviceBuffer buff = GRAPH_CHECK_RESULT(
        VKit::DeviceBuffer::Builder(GetDevice(), GetAllocator(), ToVulkanBufferFlags(flags)).SetSize(size).Build());

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
static BufferFlags fromVulkanBufferFlags(const VKit::DeviceBufferFlags flags)
{
    BufferFlags result = 0;
    if (flags & VKit::DeviceBufferFlag_DeviceLocal)
        result |= BufferFlag_DeviceLocal;
    if (flags & VKit::DeviceBufferFlag_HostVisible)
        result |= BufferFlag_HostVisible;
    if (flags & VKit::DeviceBufferFlag_Source)
        result |= BufferFlag_Source;
    if (flags & VKit::DeviceBufferFlag_Destination)
        result |= BufferFlag_Destination;
    if (flags & VKit::DeviceBufferFlag_Staging)
        result |= BufferFlag_Staging;
    if (flags & VKit::DeviceBufferFlag_Vertex)
        result |= BufferFlag_Vertex;
    if (flags & VKit::DeviceBufferFlag_Index)
        result |= BufferFlag_Index;
    if (flags & VKit::DeviceBufferFlag_Storage)
        result |= BufferFlag_Storage;
    if (flags & VKit::DeviceBufferFlag_Indirect)
        result |= BufferFlag_Indirect;
    if (flags & VKit::DeviceBufferFlag_HostMapped)
        result |= BufferFlag_HostMapped;
    if (flags & VKit::DeviceBufferFlag_HostRandomAccess)
        result |= BufferFlag_HostRandomAccess;
    return result;
}
BufferFlags Buffer_GetFlags(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    return fromVulkanBufferFlags(s_Buffers->At(Handle_GetId(buffer)).GetInfo().Flags);
}

void *Buffer_Map(const Buffer buffer)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    VKit::DeviceBuffer &buff = s_Buffers->At(Handle_GetId(buffer));
    GRAPH_CHECK_RESULT(buff.Map());
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
    GRAPH_CHECK_RESULT(s_Buffers->At(Handle_GetId(buffer)).Flush());
}
void Buffer_SetName(const Buffer buffer, const char *name)
{
    GRAPH_CHECK_HANDLE(buffer, Handle_Buffer);
    TKIT_ASSERT(IsValidationEnabled(), "[GRAPH][RESOURCES] To name objects, the validation capability must be enabled");
    GRAPH_CHECK_RESULT(s_Buffers->At(Handle_GetId(buffer)).SetName(name));
}

bool Buffer_IsHandleValid(const Buffer buffer)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Buffers, buffer, Handle_Buffer);
}

Image Image_Create(const u32v3 &size, const ImageSpecs &specs, const ImageFlags flags)
{
    TKit::StackArray<VkFormat> formats{};
    for (const Format fmt : specs.Formats)
        formats.Append(ToVulkan(fmt));

    VKit::DeviceImage::Builder builder{
        GetDevice(), GetAllocator(), {size[0], size[1], size[2]}, formats, ToVulkanImageFlags(flags)};
    builder.SetMipLevels(specs.MipLevels)
        .SetArrayLayers(specs.ArrayLayers)
        .SetImageType(ToVulkan(specs.Type))
        .SetTiling(ToVulkan(specs.Tiling))
        .SetInitialLayout(ToVulkan(specs.InitialLayout))
        .SetSamples(ToVulkan(specs.Samples));
    if (flags & ImageFlag_CubeCompatible)
        builder.SetFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

    const VKit::DeviceImage img = GRAPH_CHECK_RESULT(builder.Build());

    const Image handle = Handle_Create(Handle_Image, s_Images->Insert(img, true));
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

    GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(s_Images, img, image_Destroy);
}
void Image_DestroyViews(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    s_Images->At(Handle_GetId(img)).Image.DestroyImageViews();
}

usz Image_ComputeSize(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    return s_Images->At(Handle_GetId(img)).Image.ComputeSize();
}

ImageView Image_AddView(const Image img, const ImageViewSpecs &specs)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    VKit::DeviceImage &image = s_Images->At(Handle_GetId(img)).Image;

    VkImageViewCreateInfo info{};
    info.image = VK_NULL_HANDLE;
    info.viewType = ToVulkan(specs.Type);
    info.subresourceRange = ToVulkan(image, specs.Range);
    info.format = ToVulkan(specs.Format);
    const VkImageView view = GRAPH_CHECK_RESULT(image.AddImageView(info));

    return Handle_Create(Handle_ImageView, s_Views->Insert(img, view));
}

ImageView Image_GetView(const Image img, const u32 idx)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);

    VKit::DeviceImage &image = s_Images->At(Handle_GetId(img)).Image;
    const VkImageView view = image.GetView(idx);
    for (const Id id : s_Views->GetValidIds())
        if (s_Views->At(id).View == view)
            return Handle_Create(Handle_ImageView, id);

    TKIT_FATAL("[GRAPH][RESOURCES] Could not find the associated image view");
    return NullHandle;
}

static ImageLayout fromVulkan(const VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return ImageLayout_Undefined;
    case VK_IMAGE_LAYOUT_GENERAL:
        return ImageLayout_General;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return ImageLayout_ColorAttachment;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return ImageLayout_DepthStencilAttachment;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return ImageLayout_DepthStencilReadOnly;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return ImageLayout_ShaderReadOnly;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return ImageLayout_TransferSrc;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return ImageLayout_TransferDst;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return ImageLayout_Present;
    default:
        TKIT_FATAL("[GRAPH] Unsupported Vulkan image layout: {}", u32(layout));
        return ImageLayout_Undefined;
    }
}

void Image_SetLayout(const Image img, const ImageLayout layout)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    s_Images->At(Handle_GetId(img)).Image.Layout = ToVulkan(layout);
}
ImageLayout Image_GetLayout(const Image img)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    return fromVulkan(s_Images->At(Handle_GetId(img)).Image.Layout);
}
void Image_SetName(const Image img, const char *name)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    TKIT_ASSERT(IsValidationEnabled(), "[GRAPH][RESOURCES] To name objects, the validation capability must be enabled");
    GRAPH_CHECK_RESULT(s_Images->At(Handle_GetId(img)).Image.SetName(name));
}
void Image_SetViewName(const Image img, const char *name)
{
    GRAPH_CHECK_HANDLE(img, Handle_Image);
    GRAPH_CHECK_RESULT(s_Images->At(Handle_GetId(img)).Image.SetViewNames(name));
}
bool Image_IsHandleValid(const Image img)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Images, img, Handle_Image);
}

void ImageView_SetName(const ImageView view, const char *name)
{
    GRAPH_CHECK_HANDLE(view, Handle_ImageView);
    TKIT_ASSERT(IsValidationEnabled(), "[GRAPH][RESOURCES] To name objects, the validation capability must be enabled");
    GRAPH_CHECK_RESULT(
        GetDevice().SetObjectName(s_Views->At(Handle_GetId(view)).View, VK_OBJECT_TYPE_IMAGE_VIEW, name));
}
bool ImageView_IsHandleValid(const ImageView view)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Views, view, Handle_ImageView);
}

Sampler Sampler_Create(const SamplerSpecs &specs, const SamplerFlags flags)
{
    const VKit::Sampler smp = GRAPH_CHECK_RESULT(
        VKit::Sampler::Builder(GetDevice())
            .SetMipmapMode(ToVulkan(specs.Mode))
            .SetFilters(ToVulkan(specs.Filters[0]), ToVulkan(specs.Filters[1]))
            .SetAddressModes(ToVulkan(specs.Wraps[0]), ToVulkan(specs.Wraps[0]), ToVulkan(specs.Wraps[0]))
            .SetLodRange(specs.LodRange[0], specs.LodRange[1])
            .SetAnisotropy(specs.MaxAnisotropy)
            .SetCompareOp(ToVulkan(specs.Compare))
            .SetBorderColor(ToVulkan(specs.Border))
            .EnableAnisotropy((flags & SamplerFlag_EnableAnisotropy) != 0)
            .EnableCompare(specs.Compare != CompareOp_Always)
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
    TKIT_ASSERT(IsValidationEnabled(), "[GRAPH][RESOURCES] To name objects, the validation capability must be enabled");
    GRAPH_CHECK_RESULT(s_Samplers->At(Handle_GetId(smp)).SetName(name));
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
    return s_Images->At(Handle_GetId(img)).Image;
}
VkImageView GetImageView(const ImageView view)
{
    GRAPH_CHECK_HANDLE(view, Handle_ImageView);
    return s_Views->At(Handle_GetId(view)).View;
}
VKit::Sampler &GetSampler(const Sampler smp)
{
    GRAPH_CHECK_HANDLE(smp, Handle_Sampler);
    return s_Samplers->At(Handle_GetId(smp));
}

Image CreateNonOwnedImage(const VKit::DeviceImage &img)
{
    const Id id = s_Images->Insert(img, false);
    const Image handle = Handle_Create(Handle_Image, id);
    for (const VkImageView v : s_Images->At(id).Image.GetViews())
        s_Views->Insert(img, v);
    return handle;
}
} // namespace Graph
