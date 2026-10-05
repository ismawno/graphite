#pragma once

#include "graph/alias.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/memory/memory.hpp"
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
using Swapchain = Handle;
using CommandPool = Handle;
using CommandBuffer = Handle;
using Buffer = Handle;
using Image = Handle;
using ImageView = Handle;
using Sampler = Handle;
using DescriptorSet = Handle;
using DescriptorSetLayout = Handle;
using Shader = Handle;
using Compilation = Handle;
using Reflection = Handle;
using PipelineLayout = Handle;
using Pipeline = Handle;

enum HandleType : u8
{
    Handle_Window,
    Handle_Monitor,
    Handle_Swapchain,
    Handle_CommandPool,
    Handle_CommandBuffer,
    Handle_Buffer,
    Handle_Image,
    Handle_ImageView,
    Handle_Sampler,
    Handle_DescriptorSet,
    Handle_DescriptorSetLayout,
    Handle_Shader,
    Handle_Compilation,
    Handle_Reflection,
    Handle_PipelineLayout,
    Handle_Pipeline,
    Handle_Count,
    Handle_None = Handle_Count
};

static_assert(Handle_Count <= GRAPH_MAX_HANDLE_TYPES,
              "[GRAPH][HANDLE] The handle type count exceeds maximum handle types");

struct Allocation
{
    TKit::ArenaAllocator *Arena = nullptr;
    TKit::StackAllocator *Stack = nullptr;
    TKit::TierAllocator *Tier = nullptr;
};

using Capabilities = u16;
enum CapabilityFlagBit : Capabilities
{
    Capability_Validation = 1U << 0,
    Capability_DebugPrintf = 1U << 1,
    Capability_FaultDump = 1U << 2,
    Capability_IndependentBlend = 1U << 3,
    Capability_MultiDrawIndirect = 1U << 4,
    Capability_ShaderDrawParameters = 1U << 5,
    Capability_ExtendedDynamicState = 1U << 6,
    Capability_BindlessDescriptors = 1U << 7,
    Capability_ImageMultiFormat = 1U << 8,
    Capability_RequireGraphicsQueue = 1U << 9,
    Capability_RequireTransferQueue = 1U << 10,
    Capability_RequireComputeQueue = 1U << 11,
    Capability_All = TKit::Limits<Capabilities>::Max(),
};

#ifdef GRAPH_HAS_PLATFORM_BACKEND
enum Platform : u8
{
    Platform_Any,
    Platform_Win32,
    Platform_Cocoa,
    Platform_Wayland,
    Platform_X11,
#    ifdef TKIT_OS_LINUX
    Platform_Auto = Platform_X11,
#    elif defined(TKIT_OS_APPLE)
    Platform_Auto = Platform_Cocoa,
#    elif defined(TKIT_OS_WINDOWS)
    Platform_Auto = Platform_Win32,
#    else
    Platform_Auto = Platform_Any,
#    endif
};
#endif

using WindowFlags = u8;
enum WindowFlagBit : WindowFlags
{
    WindowFlag_Resizable = 1U << 0,
    WindowFlag_Visible = 1U << 1,
    WindowFlag_Decorated = 1U << 2,
    WindowFlag_Focused = 1U << 3,
    WindowFlag_Floating = 1U << 4,
    WindowFlag_FocusOnShow = 1U << 5,
    WindowFlag_Iconified = 1U << 6,
    WindowFlag_NoClientApi = 1U << 7,
};

enum MouseCursor : u8
{
    MouseCursor_Default,
    MouseCursor_Arrow,
    MouseCursor_NS,
    MouseCursor_EW,
    MouseCursor_NWSE,
    MouseCursor_NESW,
    MouseCursor_Hand,
    MouseCursor_CrossHair,
    MouseCursor_IBeam,
    MouseCursor_NotAllowed,
    MouseCursor_Count,
};

enum DescriptorType : u8
{
    Descriptor_StorageBuffer,
    Descriptor_UniformBuffer,
    Descriptor_Sampler,
    Descriptor_SampledImage,
    Descriptor_CombinedImageSampler,
    Descriptor_StorageImage,
    Descriptor_Count,
};

enum PipelineType : u8
{
    Pipeline_Graphics,
    Pipeline_Compute,
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

using DescriptorBindingFlags = u8;
enum DescriptorBindingFlagBit : DescriptorBindingFlags
{
    DescriptorBindingFlag_UpdateAfterBind = 1U << 0,
    DescriptorBindingFlag_UpdateUnusedWhilePending = 1U << 1,
    DescriptorBindingFlag_PartiallyBound = 1U << 2,
    DescriptorBindingFlag_VariableDescriptorCount = 1U << 3,
};

using DynamicStateFlags = u8;
enum DynamicStateFlagBit : DynamicStateFlags
{
    DynamicStateFlag_Viewport = 1U << 0,
    DynamicStateFlag_Scissor = 1U << 1,
    DynamicStateFlag_CullMode = 1U << 2,
};

enum LoadOp : u8
{
    LoadOp_Load,
    LoadOp_Clear,
    LoadOp_DontCare,
};

enum StoreOp : u8
{
    StoreOp_Store,
    StoreOp_DontCare,
};

enum ResolveMode : u8
{
    Resolve_None,
    Resolve_SampleZero,
    Resolve_Average,
    Resolve_Min,
    Resolve_Max,
};

enum QueueType : u8
{
    Queue_Graphics,
    Queue_Transfer,
    Queue_Compute,
    Queue_Count,
    Queue_None = Queue_Count
};

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

using PipelineStageFlags = u16;
enum PipelineStageFlagBit : PipelineStageFlags
{
    PipelineStageFlag_DrawIndirect = 1U << 0,
    PipelineStageFlag_VertexInput = 1U << 1,
    PipelineStageFlag_VertexShader = 1U << 2,
    PipelineStageFlag_FragmentShader = 1U << 3,
    PipelineStageFlag_EarlyFragmentTests = 1U << 4,
    PipelineStageFlag_LateFragmentTests = 1U << 5,
    PipelineStageFlag_ColorAttachmentOutput = 1U << 6,
    PipelineStageFlag_ComputeShader = 1U << 7,
    PipelineStageFlag_Transfer = 1U << 8,
    PipelineStageFlag_Host = 1U << 9,
    PipelineStageFlag_AllCommands = 1U << 10,
};

using AccessFlags = u16;
enum AccessFlagBit : AccessFlags
{
    AccessFlag_IndirectCommandRead = 1U << 0,
    AccessFlag_IndexRead = 1U << 1,
    AccessFlag_VertexAttributeRead = 1U << 2,
    AccessFlag_UniformRead = 1U << 3,
    AccessFlag_ShaderRead = 1U << 4,
    AccessFlag_ShaderWrite = 1U << 5,
    AccessFlag_ColorAttachmentRead = 1U << 6,
    AccessFlag_ColorAttachmentWrite = 1U << 7,
    AccessFlag_DepthStencilAttachmentRead = 1U << 8,
    AccessFlag_DepthStencilAttachmentWrite = 1U << 9,
    AccessFlag_TransferRead = 1U << 10,
    AccessFlag_TransferWrite = 1U << 11,
    AccessFlag_HostRead = 1U << 12,
    AccessFlag_HostWrite = 1U << 13,
    AccessFlag_MemoryRead = 1U << 14,
    AccessFlag_MemoryWrite = 1U << 15,
};

enum BindPoint : u8
{
    BindPoint_Graphics,
    BindPoint_Compute,
};

enum IndexType : u8
{
    IndexType_Unsigned8,
    IndexType_Unsigned16,
    IndexType_Unsigned32,
};

enum PresentMode : u8
{
    PresentMode_Immediate,
    PresentMode_Mailbox,
    PresentMode_VSync,
    PresentMode_Count
};

struct Specs
{
    const char *ApplicationName = "Graphite app";
    const char *LoaderPath = nullptr;
    const char *DumpPath = nullptr;
    Allocation Allocators{};
    Capabilities EnabledCapabilities = 0;
    TKit::FixedArray<u32, Descriptor_Count> DescriptorPoolSizes;
#ifdef GRAPH_HAS_PLATFORM_BACKEND
    Platform TargetPlatform = Platform_Auto;
#endif
    u32 MaxCommandPools = 8;
    u32 MaxCommandBuffers = 32;
    u32 MaxDescriptorSets = 128;
    u32 MaxBuffers = 64;
    u32 MaxImages = 256;
    u32 MaxSamplers = 16;
    u32 MaxImageViews = 1024;
    u32 MaxShaders = 16;
    u32 MaxCompilations = 4;
    u32 MaxReflections = 4;
    u32 MaxPipelineLayouts = 32;
    u32 MaxPipelines = 256;

    Specs()
    {
        for (u32 i = 0; i < Descriptor_Count; ++i)
            DescriptorPoolSizes[i] = 128;
    }
};

void Initialize(const Specs &specs);
void Terminate();

const char *ToString(HandleType htype);
void DeviceWaitIdle();
bool IsValidationEnabled();

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

} // namespace Graph
