#pragma once

#include "graph/alias.hpp"
#include "tkit/utils/limits.hpp"
#include "tkit/utils/debug.hpp"

#define GRAPH_HANDLE_WIDTH 32U
#define GRAPH_HANDLE_MASK (~0U)

///

#define GRAPH_HANDLE_TYPE_BITS 5U

#define GRAPH_HANDLE_TYPE_SHIFT (GRAPH_HANDLE_WIDTH - GRAPH_HANDLE_TYPE_BITS)
#define GRAPH_HANDLE_ID_BITS GRAPH_HANDLE_TYPE_SHIFT

#define GRAPH_HANDLE_TYPE_MASK (GRAPH_HANDLE_MASK << GRAPH_HANDLE_TYPE_SHIFT)
#define GRAPH_HANDLE_ID_MASK ~GRAPH_HANDLE_TYPE_MASK

#define GRAPH_NULL_HANDLE_ID GRAPH_HANDLE_ID_MASK

#define GRAPH_MAX_HANDLE_TYPES ((1U << GRAPH_HANDLE_TYPE_BITS) - 1U)
#define GRAPH_MAX_HANDLE_IDS ((1U << GRAPH_HANDLE_ID_BITS) - 1U)

#ifdef TKIT_ENABLE_ENSURE
#    define GRAPH_CHECK_HANDLE_HAS_VALID_TYPE(hndl)                                                                    \
        TKIT_ASSERT(Graph::Handle_GetTypeAsInteger(hndl) < Graph::Handle_Count,                                        \
                    "[GRAPH][HANDLE] The handle {:#010x} does not match any known handle types ({}), which likely "    \
                    "means it is a "                                                                                   \
                    "broken handle",                                                                                   \
                    hndl, Graph::Handle_GetTypeAsInteger(hndl))

#    define __GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype)                                                                 \
        TKIT_ASSERT(Graph::Handle_GetType(hndl) == htype,                                                              \
                    "[GRAPH][HANDLE] The handle {:#010x} is not a '{}' handle, but rather a '{}' handle", hndl,        \
                    Graph::ToString(htype), Graph::ToString(Graph::Handle_GetType(hndl)))

#    define GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype)                                                                   \
        GRAPH_CHECK_HANDLE_HAS_VALID_TYPE(hndl);                                                                       \
        __GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype)

#    define GRAPH_CHECK_ID_IS_NOT_NULL(hndl)                                                                           \
        TKIT_ASSERT(!Graph::Handle_IsIdNull(hndl), "[GRAPH][HANDLE] The handle {:#010x} has a null id", hndl)

#    define GRAPH_CHECK_HANDLE_IS_VALID(hndl, htype)                                                                   \
        GRAPH_CHECK_HANDLE_HAS_TYPE(hndl, htype);                                                                      \
        TKIT_ASSERT(Graph::Handle_IsValid(hndl, htype),                                                                \
                    "[GRAPH][HANDLE] The handle {:#010x} is not a valid '{}' handle", hndl, ToString(htype))

#    define GRAPH_CHECK_HANDLE(hndl, htype)                                                                            \
        GRAPH_CHECK_ID_IS_NOT_NULL(hndl);                                                                              \
        GRAPH_CHECK_HANDLE_IS_VALID(hndl, htype)

#else
#    define GRAPH_CHECK_HANDLE_HAS_VALID_TYPE(...)
#    define GRAPH_CHECK_HANDLE_HAS_VALID_RESOURCE_POOL_TYPE(...)
#    define GRAPH_CHECK_HANDLE_HAS_TYPE(...)
#    define GRAPH_CHECK_HANDLE_HAS_RESOURCE_POOL_TYPE(...)
#    define GRAPH_CHECK_RESOURCE_IS_NOT_NULL(...)
#    define GRAPH_CHECK_RESOURCE_POOL_IS_NOT_NULL(...)
#    define GRAPH_CHECK_RESOURCE_POOL_ID_IS_NOT_NULL(...)
#    define GRAPH_CHECK_RESOURCE_IS_VALID(...)
#    define GRAPH_CHECK_RESOURCE_IS_VALID_WITH_DIM(...)
#    define GRAPH_CHECK_RESOURCE_POOL_IS_VALID(...)
#    define GRAPH_CHECK_RESOURCE_POOL_IS_VALID_WITH_DIM(...)
#endif

namespace Graph
{
using Handle = u<GRAPH_HANDLE_WIDTH>;
constexpr Handle NullHandle = TKit::Limits<Handle>::Max();

using Id = Handle;
constexpr Id NullId = GRAPH_NULL_HANDLE_ID;

using Window = Handle;
using Monitor = Handle;
using Surface = Handle;
using Queue = Handle;
using CommandPool = Handle;
using CommandBuffer = Handle;
using Buffer = Handle;
using Image = Handle;
using ImageView = Handle;
using Sampler = Handle;
using Shader = Handle;
using Compilation = Handle;
using PipelineLayout = Handle;
using Pipeline = Handle;

enum HandleType : u8
{
    Handle_Window,
    Handle_Monitor,
    Handle_Surface,
    Handle_Queue,
    Handle_CommandPool,
    Handle_CommandBuffer,
    Handle_Buffer,
    Handle_Image,
    Handle_ImageView,
    Handle_Sampler,
    Handle_Shader,
    Handle_Compilation,
    Handle_PipelineLayout,
    Handle_Pipeline,
    Handle_Count,
    Handle_None = Handle_Count
};

static_assert(Handle_Count <= GRAPH_MAX_HANDLE_TYPES,
              "[GRAPH][HANDLE] The handle type count exceeds maximum handle types");

const char *ToString(HandleType htype);

constexpr u32 Handle_GetTypeAsInteger(const Handle handle)
{
    return (handle & GRAPH_HANDLE_TYPE_MASK) >> GRAPH_HANDLE_TYPE_SHIFT;
}

constexpr HandleType Handle_GetType(const Handle handle)
{
    return HandleType(Handle_GetTypeAsInteger(handle));
}

constexpr bool Handle_IsIdNull(const Handle handle)
{
    return (handle & GRAPH_HANDLE_ID_MASK) == NullId;
}

constexpr Id Handle_GetId(const Handle handle)
{
    return handle & GRAPH_HANDLE_ID_MASK;
}

constexpr Handle Handle_Create(const HandleType htype, const Id id)
{
    TKIT_ASSERT(htype < Handle_Count, "[GRAPH][HANDLE] Cannot create a handle with an invalid type");
    TKIT_ASSERT(id <= GRAPH_MAX_HANDLE_IDS,
                "[GRAPH][HANDLE] Cannot create a handle with an id ({:#010x}) "
                "that exceeds the maximum bits allocated "
                "for it, as it would break the handle",
                id);
    return (u32(htype) << GRAPH_HANDLE_TYPE_SHIFT) | id;
}

bool Handle_IsValid(Handle handle, HandleType htype = Handle_None);

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

enum Format : u8
{
    Format_Undefined,
    // 8-bit single channel
    Format_R8_UNORM,
    Format_R8_SNORM,
    Format_R8_UINT,
    Format_R8_SINT,
    Format_R8_SRGB,

    // 8-bit dual channel
    Format_R8G8_UNORM,
    Format_R8G8_SNORM,
    Format_R8G8_UINT,
    Format_R8G8_SINT,
    Format_R8G8_SRGB,

    // 8-bit RGB
    Format_R8G8B8_UNORM,
    Format_R8G8B8_SNORM,
    Format_R8G8B8_UINT,
    Format_R8G8B8_SINT,
    Format_R8G8B8_SRGB,

    // 8-bit RGBA
    Format_R8G8B8A8_UNORM,
    Format_R8G8B8A8_SNORM,
    Format_R8G8B8A8_UINT,
    Format_R8G8B8A8_SINT,
    Format_R8G8B8A8_SRGB,

    // 8-bit BGRA
    Format_B8G8R8A8_UNORM,
    Format_B8G8R8A8_SNORM,
    Format_B8G8R8A8_UINT,
    Format_B8G8R8A8_SINT,
    Format_B8G8R8A8_SRGB,

    // 16-bit single channel
    Format_R16_UNORM,
    Format_R16_SNORM,
    Format_R16_UINT,
    Format_R16_SINT,
    Format_R16_SFLOAT,

    // 16-bit dual channel
    Format_R16G16_UNORM,
    Format_R16G16_SNORM,
    Format_R16G16_UINT,
    Format_R16G16_SINT,
    Format_R16G16_SFLOAT,

    // 16-bit RGB
    Format_R16G16B16_UNORM,
    Format_R16G16B16_SNORM,
    Format_R16G16B16_UINT,
    Format_R16G16B16_SINT,
    Format_R16G16B16_SFLOAT,

    // 16-bit RGBA
    Format_R16G16B16A16_UNORM,
    Format_R16G16B16A16_SNORM,
    Format_R16G16B16A16_UINT,
    Format_R16G16B16A16_SINT,
    Format_R16G16B16A16_SFLOAT,

    // 32-bit single channel
    Format_R32_UINT,
    Format_R32_SINT,
    Format_R32_SFLOAT,

    // 32-bit dual channel
    Format_R32G32_UINT,
    Format_R32G32_SINT,
    Format_R32G32_SFLOAT,

    // 32-bit RGB
    Format_R32G32B32_UINT,
    Format_R32G32B32_SINT,
    Format_R32G32B32_SFLOAT,

    // 32-bit RGBA
    Format_R32G32B32A32_UINT,
    Format_R32G32B32A32_SINT,
    Format_R32G32B32A32_SFLOAT,

    // Packed HDR
    Format_B10G11R11_UnsignedFloat,

    // Depth
    Format_D16_UNORM,
    Format_D32_SFLOAT,

    // Depth + Stencil
    Format_D24_UNORM_S8_UINT,
    Format_D32_SFLOAT_S8_UINT,

    // Compressed
    Format_BC1_RGBA_UNORM,
    Format_BC1_RGBA_SRGB,
    Format_BC5_UNORM,
    Format_BC5_SNORM,
    Format_BC7_UNORM,
    Format_BC7_SRGB,

    Format_Count,
    Format_Auto = Format_Count,
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

enum CompareOp : u8
{
    CompareOp_Never = 0,
    CompareOp_Less = 1,
    CompareOp_Equal = 2,
    CompareOp_LessOrEqual = 3,
    CompareOp_Greater = 4,
    CompareOp_NotEqual = 5,
    CompareOp_GreaterOrEqual = 6,
    CompareOp_Always = 7,
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
enum Topology : u8
{
    Topology_PointList,
    Topology_LineList,
    Topology_LineStrip,
    Topology_TriangleList,
    Topology_TriangleStrip,
    Topology_TriangleFan,
};

enum PolygonMode : u8
{
    PolygonMode_Fill,
    PolygonMode_Line,
    PolygonMode_Point,
};

enum CullMode : u8
{
    CullMode_None,
    CullMode_Front,
    CullMode_Back,
    CullMode_FrontAndBack,
};

enum FrontFace : u8
{
    FrontFace_CounterClockwise,
    FrontFace_Clockwise,
};

enum BlendFactor : u8
{
    BlendFactor_Zero,
    BlendFactor_One,
    BlendFactor_SrcColor,
    BlendFactor_OneMinusSrcColor,
    BlendFactor_DstColor,
    BlendFactor_OneMinusDstColor,
    BlendFactor_SrcAlpha,
    BlendFactor_OneMinusSrcAlpha,
    BlendFactor_DstAlpha,
    BlendFactor_OneMinusDstAlpha,
};

enum BlendOp : u8
{
    BlendOp_Add,
    BlendOp_Subtract,
    BlendOp_ReverseSubtract,
    BlendOp_Min,
    BlendOp_Max,
};

using ColorWriteMask = u8;
enum ColorWriteMaskBit : u8
{
    ColorWrite_R = 1U << 0,
    ColorWrite_G = 1U << 1,
    ColorWrite_B = 1U << 2,
    ColorWrite_A = 1U << 3,
    ColorWrite_All = ColorWrite_R | ColorWrite_G | ColorWrite_B | ColorWrite_A,
};

enum StencilOp : u8
{
    StencilOp_Keep,
    StencilOp_Zero,
    StencilOp_Replace,
    StencilOp_IncrementAndClamp,
    StencilOp_DecrementAndClamp,
    StencilOp_Invert,
    StencilOp_IncrementAndWrap,
    StencilOp_DecrementAndWrap,
};

enum VertexInputRate : u8
{
    VertexInputRate_Vertex,
    VertexInputRate_Instance,
};

using ShaderStageFlags = u8;
enum ShaderStageFlagBit : u8
{
    ShaderStageFlag_None = 0,
    ShaderStageFlag_Vertex = 1U << 0,
    ShaderStageFlag_Fragment = 1U << 1,
    ShaderStageFlag_Compute = 1U << 2,
};
} // namespace Graph
