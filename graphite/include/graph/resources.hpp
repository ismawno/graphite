#pragma once

#include "graph/core.hpp"
#include "tkit/container/span.hpp"

namespace Graph
{
struct BufferCopy
{
    usz Size;
    usz SrcOffset = 0;
    usz DstOffset = 0;
};

#define GRAPH_WHOLE_THING TKIT_U32_MAX

struct ImageSubresourceLayers
{
    u32 MipLevel = 0;
    u32 LayerStart = 0;
    u32 LayerCount = GRAPH_WHOLE_THING;
    ImageAspectFlags Aspect = ImageAspectFlag_Auto;
};

struct ImageSubresourceRange
{
    u32 MipStart = 0;
    u32 MipCount = GRAPH_WHOLE_THING;
    u32 LayerStart = 0;
    u32 LayerCount = GRAPH_WHOLE_THING;
    ImageAspectFlags Aspect = ImageAspectFlag_Auto;
};

struct BufferImageCopy
{
    usz BufferOffset = 0;
    u32 BufferRowLength = 0;
    u32 BufferImageHeight = 0;
    ImageSubresourceLayers Layers{};
    u32v3 ImageOffset{0};
    u32v3 ImageExtent{GRAPH_WHOLE_THING};
};

struct ImageBlit
{
    ImageAspectFlags Aspect = ImageAspectFlag_Auto;
    u32 SrcMip = 0;
    u32 DstMip = 0;
    u32 SrcLayerStart = 0;
    u32 DstLayerStart = 0;
    u32 LayerCount = 1;
    u32v3 SrcMin{0};
    u32v3 SrcMax{GRAPH_WHOLE_THING};
    u32v3 DstMin{0};
    u32v3 DstMax{GRAPH_WHOLE_THING};
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

ImageView Image_AddView(Image img, const ImageViewSpecs &specs = {});
ImageView Image_GetView(Image img, u32 idx = 0);

void Image_SetLayout(Image img, ImageLayout layout);
ImageLayout Image_GetLayout(Image img);

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
