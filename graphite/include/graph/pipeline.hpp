#pragma once

#include "graph/handle.hpp"
#include "tkit/container/span.hpp"

namespace Graph
{
struct StencilOpState
{
    StencilOp FailOp = StencilOp_Keep;
    StencilOp PassOp = StencilOp_Keep;
    StencilOp DepthFailOp = StencilOp_Keep;
    CompareOp CompareOp = CompareOp_Always;
    u8 CompareMask = 0;
    u8 WriteMask = 0;
    u8 Reference = 0;
};

struct ColorAttachment
{
    BlendFactor SrcColor = BlendFactor_SrcAlpha;
    BlendFactor DstColor = BlendFactor_OneMinusSrcAlpha;
    BlendOp ColorOp = BlendOp_Add;
    BlendFactor SrcAlpha = BlendFactor_One;
    BlendFactor DstAlpha = BlendFactor_OneMinusSrcAlpha;
    BlendOp AlphaOp = BlendOp_Add;
    ColorWriteMask WriteMask = ColorWrite_All;
    bool BlendEnabled = false;
};

struct VertexBinding
{
    u32 Stride = 0;
    VertexInputRate InputRate = VertexInputRate_Vertex;
};

struct VertexAttribute
{
    u32 Binding = 0;
    u32 Offset = 0;
    Format Format = Format_Undefined;
};

struct SpecializationEntry
{
    u32 ConstantID = 0;
    u32 Offset = 0;
    u32 Size = 0;
};

struct SpecializationInfo
{
    TKit::Span<const SpecializationEntry> Entries{};
    const void *Data = nullptr;
    u32 DataSize = 0;
};

struct ShaderStageInfo
{
    const char *EntryPoint = "main";
    Shader Module = NullHandle;
    SpecializationInfo Specialization{};
    ShaderStageFlagBit Stage = ShaderStageFlag_Vertex;
};

struct DepthStencilState
{
    StencilOpState Front{};
    StencilOpState Back{};
    f32 MinDepthBounds = 0.f;
    f32 MaxDepthBounds = 1.f;
    CompareOp DepthCompareOp = CompareOp_Less;
    bool DepthBoundsTestEnabled = false;
    bool DepthTestEnabled = true;
    bool DepthWriteEnabled = true;
    bool StencilTestEnabled = false;
};

struct RasterizationState
{
    f32 DepthBiasConstantFactor = 0.f;
    f32 DepthBiasClamp = 0.f;
    f32 DepthBiasSlopeFactor = 0.f;
    f32 LineWidth = 1.f;
    PolygonMode PolygonMode = PolygonMode_Fill;
    CullMode CullMode = CullMode_Back;
    FrontFace FrontFace = FrontFace_CounterClockwise;
    bool DepthBiasEnabled = false;
};

struct MultisampleState
{
    f32 MinSampleShading = 1.f;
    SampleCount Samples = SampleCount_1;
    bool SampleShadingEnabled = false;
    bool AlphaToCoverageEnabled = false;
    bool AlphaToOneEnabled = false;
};

struct GraphicsPipelineSpecs
{
    TKit::Span<const ShaderStageInfo> ShaderStages{};
    TKit::Span<const VertexBinding> VertexBindings{};
    TKit::Span<const VertexAttribute> VertexAttributes{};
    TKit::Span<const ColorAttachment> ColorAttachments{};
    TKit::Span<const Format> ColorAttachmentFormats{};

    Topology Topology = Topology_TriangleList;
    RasterizationState Rasterization{};
    MultisampleState Multisample{};
    DepthStencilState DepthStencil{};

    PipelineLayout Layout = NullHandle;

    Format DepthFormat = Format_Undefined;
    Format StencilFormat = Format_Undefined;
};
} // namespace Graph
