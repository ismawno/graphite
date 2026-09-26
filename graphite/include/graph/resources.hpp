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
    Format Format = Format_Auto;
    ImageViewType Type = ImageViewType_Auto;
};

struct ImageSpecs
{
    TKit::Span<const ImageViewSpecs> ImageViews{};
    TKit::Span<const Format> Formats{};
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
    CompareOp Compare = CompareOp_Always;
    BorderColor Border = BorderColor_IntOpaqueBlack;
};

Sampler Sampler_Create(const SamplerSpecs &specs, SamplerFlags flags = 0);
void Sampler_Destroy(Sampler smp);

void Sampler_SetName(Sampler smp, const char *name);
bool Sampler_IsHandleValid(Sampler smp);

} // namespace Graph
