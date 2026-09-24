#pragma once

#include "graph/handle.hpp"
#include "tkit/container/span.hpp"

namespace Graph
{
using BufferFlags = u16;
enum BufferFlagBit : BufferFlags
{
    BufferFlag_DeviceLocal = 1U << 0,
    BufferFlag_HostVisible = 1U << 1,
    BufferFlag_Source = 1U << 2,
    BufferFlag_Destination = 1U << 3,
    BufferFlag_Staging = 1U << 4,
    BufferFlag_Vertex = 1U << 5,
    BufferFlag_Index = 1U << 6,
    BufferFlag_Storage = 1U << 7,
    BufferFlag_Indirect = 1U << 8,
    BufferFlag_HostMapped = 1U << 9,
    BufferFlag_HostRandomAccess = 1U << 10,
};

struct BufferCopy
{
    usz Size;
    usz SrcOffset = 0;
    usz DstOffset = 0;
};

Buffer Buffer_Create(usz size, BufferFlags flags);
template <typename T> Buffer Buffer_Create(const u32 count, const BufferFlags flags)
{
    return Buffer_Create(count * sizeof(T), flags);
}
void Buffer_Destroy(Buffer buffer);

usz Buffer_GetSize(Buffer buffer);
BufferFlags Buffer_GetFlags(Buffer buffer);

void *Buffer_Map(Buffer buffer);
void Buffer_Unmap(Buffer buffer);
void *Buffer_GetData(Buffer buffer);

bool Buffer_IsMapped(Buffer buffer);

void Buffer_Write(Buffer buffer, const void *data, const BufferCopy &copy);
void Buffer_Write(const Buffer buffer, const void *data, const usz size)
{
    Buffer_Write(buffer, data, {.Size = size});
}

void Buffer_Flush(Buffer buffer);

void Buffer_SetName(Buffer buffer, const char *name);
bool Buffer_IsHandleValid(Buffer buffer);

using ImageFlags = u16;
enum ImageFlagBit : ImageFlags
{
    ImageFlag_Color = 1U << 0,
    ImageFlag_Depth = 1U << 1,
    ImageFlag_Stencil = 1U << 2,
    ImageFlag_ColorAttachment = 1U << 3,
    ImageFlag_DepthAttachment = 1U << 4,
    ImageFlag_StencilAttachment = 1U << 5,
    ImageFlag_InputAttachment = 1U << 6,
    ImageFlag_Sampled = 1U << 7,
    ImageFlag_Storage = 1U << 8,
    ImageFlag_ForceHostVisible = 1U << 9,
    ImageFlag_Source = 1U << 10,
    ImageFlag_Destination = 1U << 11,
    ImageFlag_CubeCompatible = 1U << 12,
};

enum ImageTiling : u8
{
    ImageTiling_Optimal,
    ImageTiling_Linear,
};

enum ImageLayout : u8
{
    ImageLayout_Undefined,
    ImageLayout_General,
    ImageLayout_ColorAttachment,
    ImageLayout_DepthStencilAttachment,
    ImageLayout_DepthStencilReadOnly,
    ImageLayout_ShaderReadOnly,
    ImageLayout_TransferSrc,
    ImageLayout_TransferDst,
    ImageLayout_Present,
};

enum SampleCount : u8
{
    SampleCount_1,
    SampleCount_2,
    SampleCount_4,
    SampleCount_8,
    SampleCount_16,
    SampleCount_32,
    SampleCount_64,
};

enum ImageFormat : u8
{
    ImageFormat_Undefined,
    // 8-bit single channel
    ImageFormat_R8_UNORM,
    ImageFormat_R8_SNORM,
    ImageFormat_R8_UINT,
    ImageFormat_R8_SINT,
    ImageFormat_R8_SRGB,

    // 8-bit dual channel
    ImageFormat_R8G8_UNORM,
    ImageFormat_R8G8_SNORM,
    ImageFormat_R8G8_UINT,
    ImageFormat_R8G8_SINT,
    ImageFormat_R8G8_SRGB,

    // 8-bit RGB
    ImageFormat_R8G8B8_UNORM,
    ImageFormat_R8G8B8_SNORM,
    ImageFormat_R8G8B8_UINT,
    ImageFormat_R8G8B8_SINT,
    ImageFormat_R8G8B8_SRGB,

    // 8-bit RGBA
    ImageFormat_R8G8B8A8_UNORM,
    ImageFormat_R8G8B8A8_SNORM,
    ImageFormat_R8G8B8A8_UINT,
    ImageFormat_R8G8B8A8_SINT,
    ImageFormat_R8G8B8A8_SRGB,

    // 8-bit BGRA
    ImageFormat_B8G8R8A8_UNORM,
    ImageFormat_B8G8R8A8_SNORM,
    ImageFormat_B8G8R8A8_UINT,
    ImageFormat_B8G8R8A8_SINT,
    ImageFormat_B8G8R8A8_SRGB,

    // 16-bit single channel
    ImageFormat_R16_UNORM,
    ImageFormat_R16_SNORM,
    ImageFormat_R16_UINT,
    ImageFormat_R16_SINT,
    ImageFormat_R16_SFLOAT,

    // 16-bit dual channel
    ImageFormat_R16G16_UNORM,
    ImageFormat_R16G16_SNORM,
    ImageFormat_R16G16_UINT,
    ImageFormat_R16G16_SINT,
    ImageFormat_R16G16_SFLOAT,

    // 16-bit RGB
    ImageFormat_R16G16B16_UNORM,
    ImageFormat_R16G16B16_SNORM,
    ImageFormat_R16G16B16_UINT,
    ImageFormat_R16G16B16_SINT,
    ImageFormat_R16G16B16_SFLOAT,

    // 16-bit RGBA
    ImageFormat_R16G16B16A16_UNORM,
    ImageFormat_R16G16B16A16_SNORM,
    ImageFormat_R16G16B16A16_UINT,
    ImageFormat_R16G16B16A16_SINT,
    ImageFormat_R16G16B16A16_SFLOAT,

    // 32-bit single channel
    ImageFormat_R32_UINT,
    ImageFormat_R32_SINT,
    ImageFormat_R32_SFLOAT,

    // 32-bit dual channel
    ImageFormat_R32G32_UINT,
    ImageFormat_R32G32_SINT,
    ImageFormat_R32G32_SFLOAT,

    // 32-bit RGB
    ImageFormat_R32G32B32_UINT,
    ImageFormat_R32G32B32_SINT,
    ImageFormat_R32G32B32_SFLOAT,

    // 32-bit RGBA
    ImageFormat_R32G32B32A32_UINT,
    ImageFormat_R32G32B32A32_SINT,
    ImageFormat_R32G32B32A32_SFLOAT,

    // Packed HDR
    ImageFormat_B10G11R11_UnsignedFloat,

    // Depth
    ImageFormat_D16_UNORM,
    ImageFormat_D32_SFLOAT,

    // Depth + Stencil
    ImageFormat_D24_UNORM_S8_UINT,
    ImageFormat_D32_SFLOAT_S8_UINT,

    // Compressed
    ImageFormat_BC1_RGBA_UNORM,
    ImageFormat_BC1_RGBA_SRGB,
    ImageFormat_BC5_UNORM,
    ImageFormat_BC5_SNORM,
    ImageFormat_BC7_UNORM,
    ImageFormat_BC7_SRGB,

    ImageFormat_Count,
    ImageFormat_Auto = ImageFormat_Count,
};

enum ImageType : u8
{
    ImageType_1D,
    ImageType_2D,
    ImageType_3D,
};

enum ImageViewType : u8
{
    ImageViewType_1D,
    ImageViewType_2D,
    ImageViewType_3D,
    ImageViewType_Auto,
};

using ImageAspectFlags = u8;
enum ImageAspectFlagBit : ImageAspectFlags
{
    ImageAspectFlag_Color = 1U << 0,
    ImageAspectFlag_Depth = 1U << 1,
    ImageAspectFlag_Stencil = 1U << 2,
    ImageAspectFlag_Auto = 1U << 3,
};

#define GRAPH_IMAGE_SUBRESOURCE_RANGE_AUTO TKIT_U32_MAX

struct ImageSubresourceRange
{
    ImageAspectFlags Aspect = ImageAspectFlag_Auto;
    u32 MipStart = GRAPH_IMAGE_SUBRESOURCE_RANGE_AUTO;
    u32 MipCount = GRAPH_IMAGE_SUBRESOURCE_RANGE_AUTO;
    u32 LayerStart = GRAPH_IMAGE_SUBRESOURCE_RANGE_AUTO;
    u32 LayerCount = GRAPH_IMAGE_SUBRESOURCE_RANGE_AUTO;
};

struct ImageViewSpecs
{
    ImageSubresourceRange Range{};
    ImageFormat Format = ImageFormat_Auto;
    ImageViewType Type = ImageViewType_Auto;
};

struct ImageSpecs
{
    TKit::Span<const ImageViewSpecs> ImageViews{};
    TKit::Span<const ImageFormat> Formats{};
    u32 MipLevels = 1;
    u32 ArrayLayers = 1;
    ImageType Type = ImageType_2D;
    ImageTiling Tiling = ImageTiling_Optimal;
    ImageLayout InitialLayout = ImageLayout_Undefined;
    SampleCount Samples = SampleCount_1;
};

Image Image_Create(const u32v3 &size, const ImageSpecs &specs, ImageFlags flags);
Image Image_Create(const u32v2 &size, const ImageSpecs &specs, const ImageFlags flags)
{
    return Image_Create(u32v3{size, 1}, specs, flags);
}

void Image_Destroy(Image img);
void Image_DestroyViews(Image img);

usz Image_ComputeSize(Image img);

ImageView Image_AddView(Image img, const ImageViewSpecs &specs);
ImageView Image_GetView(Image img, u32 idx = 0);

void Image_SetLayout(Image img, ImageLayout layout);

void Image_SetName(Image img, const char *name);
void Image_SetViewNames(Image img, const char *name);
bool Image_IsHandleValid(Image img);

void ImageView_SetName(ImageView view, const char *name);
bool ImageView_IsHandleValid(ImageView view);

enum SamplerMode : u8
{
    SamplerMode_Linear,
    SamplerMode_Nearest,
};

enum Filter : u8
{
    Filter_Linear,
    Filter_Nearest,
    Filter_Cubic,
};

enum Wrap : u8
{
    Wrap_Repeat,
    Wrap_ClampToEdge,
    Wrap_MirroredRepeat,
};

enum CompareOperation : u8
{
    Compare_Never = 0,
    Compare_Less = 1,
    Compare_Equal = 2,
    Compare_LessOrEqual = 3,
    Compare_Greater = 4,
    Compare_NotEqual = 5,
    Compare_GreaterOrEqual = 6,
    Compare_Always = 7,
};

enum BorderColor : u8
{
    BorderColor_FloatTransparentBlack,
    BorderColor_IntTransparentBlack,
    BorderColor_FloatOpaqueBlack,
    BorderColor_IntOpaqueBlack,
    BorderColor_FloatOpaqueWhite,
    BorderColor_IntOpaqueWhite,
};

using SamplerFlags = u8;
enum SamplerFlagBit : SamplerFlags
{
    SamplerFlag_EnableAnisotropy = 1U << 0,
    SamplerFlag_EnableUnnormalizedCoordinates = 1U << 1,
};

struct SamplerSpecs
{
    SamplerMode Mode = SamplerMode_Linear;

    // [0] -> mag, [1] -> min
    vec2<Filter> Filters = Filter_Linear;
    vec3<Wrap> Wraps = Wrap_ClampToEdge;
    f32v2 LodRange = {0.f, 1000.f};
    f32 MaxAnisotropy = 1.f;
    CompareOperation Compare = Compare_Always;
    BorderColor Border = BorderColor_IntOpaqueBlack;
};

Sampler Sampler_Create(const SamplerSpecs &specs, SamplerFlags flags = 0);
void Sampler_Destroy(Sampler smp);

void Sampler_SetName(Sampler smp, const char *name);
bool Sampler_IsHandleValid(Sampler smp);

} // namespace Graph
