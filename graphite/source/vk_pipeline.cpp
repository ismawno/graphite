#include "pch.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "graph/pipeline.hpp"
#include "vkit/state/pipeline_layout.hpp"
#include "vkit/state/graphics_pipeline.hpp"
#include "vkit/state/compute_pipeline.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"
#include "tkit/utils/union.hpp"

namespace Graph
{
struct Vulkan_Pipeline
{
    TKit::Union<VKit::GraphicsPipeline, VKit::ComputePipeline> Pipeline{};
    PipelineType Type;
};
static TKit::Storage<TKit::ArenaHive<VKit::PipelineLayout>> s_Layouts{};
static TKit::Storage<TKit::ArenaHive<Vulkan_Pipeline>> s_Pipelines{};

template <typename F> static void pipeline_Visit(Vulkan_Pipeline &pip, F &&fun)
{
    if (pip.Type == Pipeline_Graphics)
        fun(pip.Pipeline.Get<VKit::GraphicsPipeline>());
    else
        fun(pip.Pipeline.Get<VKit::ComputePipeline>());
}

static void pipeline_Destroy(Vulkan_Pipeline &pip)
{
    pipeline_Visit(pip, [](auto &p) { p.Destroy(); });
}

void Pipeline_Initialize(const u32 maxLayouts, const u32 maxPipelines)
{
    s_Layouts.Construct();
    s_Pipelines.Construct();

    s_Layouts->Reserve(maxLayouts);
    s_Pipelines->Reserve(maxPipelines);
}

void Pipeline_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING(s_Layouts, "PIPELINE", "pipeline layouts");
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(s_Pipelines, "PIPELINE", "pipelines", pipeline_Destroy);

    s_Layouts.Destruct();
    s_Pipelines.Destruct();
}

PipelineLayout PipelineLayout_Create(const PipelineLayoutSpecs &specs)
{
    VKit::PipelineLayout::Builder builder{GetDevice()};
    for (const DescriptorSetLayout layout : specs.DescriptorSetLayouts)
        builder.AddDescriptorSetLayout(GetDescriptorSetLayout(layout));
    for (const PushConstantRange &range : specs.PushRanges)
        builder.AddPushConstantRange(ToVulkanShaderStageFlags(range.Stages), range.Size, range.Offset);

    const VKit::PipelineLayout layout = GRAPH_CHECK_VKIT_RESULT(builder.Build());
    return Handle_Create(Handle_PipelineLayout, layout);
}

void PipelineLayout_Destroy(const PipelineLayout layout)
{
    GRAPH_CHECK_HANDLE(layout, Handle_PipelineLayout);
    GRAPH_DESTROY_FUNCTION_BODY(s_Layouts, layout);
}

void PipelineLayout_SetName(const PipelineLayout layout, const char *name)
{
    GRAPH_CHECK_HANDLE(layout, Handle_PipelineLayout);
    GRAPH_CHECK_VKIT_RESULT(s_Layouts->At(Handle_GetId(layout)).SetName(name));
}

bool PipelineLayout_IsHandleValid(const PipelineLayout layout)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Layouts, layout, Handle_PipelineLayout);
}

Pipeline Pipeline_CreateGraphics(const PipelineLayout layout, const GraphicsPipelineSpecs &specs)
{
    TKit::StackArray<VkFormat> formats{};
    formats.Reserve(specs.ColorAttachmentFormats.GetSize());
    for (const Format fmt : specs.ColorAttachmentFormats)
        formats.Append(ToVulkan(fmt));

    VkPipelineRenderingCreateInfoKHR rinfo{};
    rinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rinfo.colorAttachmentCount = specs.ColorAttachments.GetSize();
    rinfo.pColorAttachmentFormats = formats.GetData();
    rinfo.depthAttachmentFormat = ToVulkan(specs.DepthFormat);
    rinfo.stencilAttachmentFormat = ToVulkan(specs.StencilFormat);

    VKit::GraphicsPipeline::Builder builder{GetDevice(), GetPipelineLayout(layout), rinfo};

    TKit::StackArray<VkSpecializationInfo> specInfos{};
    specInfos.Reserve(specs.ShaderStages.GetSize());

    TKit::StackArray<VkSpecializationMapEntry> specEntries{};
    specEntries.Reserve(10 * specs.ShaderStages.GetSize());
    for (const ShaderStageInfo &shInfo : specs.ShaderStages)
    {
        const SpecializationInfo spInfo = shInfo.Specialization;
        VkSpecializationInfo *specInfo = shInfo.Specialization.Data ? &specInfos.Append() : nullptr;
        if (specInfo)
        {
            specInfo->dataSize = usz(spInfo.DataSize);
            specInfo->pData = spInfo.Data;
            specInfo->mapEntryCount = spInfo.Entries.GetSize();
            specInfo->pMapEntries = specEntries.end();
            for (const SpecializationEntry &entry : shInfo.Specialization.Entries)
                specEntries.Append(entry.ConstantID, entry.Offset, usz(entry.Size));
        }
        builder.AddShaderStage(GetShader(shInfo.Shader), VkShaderStageFlagBits(ToVulkanShaderStageFlags(shInfo.Stage)),
                               0, specInfo, shInfo.EntryPoint);
    }
    for (const VertexBinding &vbinding : specs.VertexBindings)
        builder.AddBindingDescription(vbinding.Stride, ToVulkan(vbinding.InputRate));
    for (const VertexAttribute &vatt : specs.VertexAttributes)
        builder.AddAttributeDescription(vatt.Binding, ToVulkan(vatt.Format), vatt.Offset, vatt.Location);
    for (const ColorAttachment &catt : specs.ColorAttachments)
    {
        builder.BeginColorAttachment()
            .EnableBlending(catt.BlendEnabled)
            .SetColorBlendFactors(ToVulkan(catt.SrcColor), ToVulkan(catt.DstColor))
            .SetColorBlendOperation(ToVulkan(catt.ColorOp))
            .SetAlphaBlendFactors(ToVulkan(catt.SrcAlpha), ToVulkan(catt.DstAlpha))
            .SetAlphaBlendOperation(ToVulkan(catt.AlphaOp))
            .SetColorWriteMask(ToVulkanColorWriteMask(catt.WriteMask))
            .EndColorAttachment();
    }
    const RasterizationState &r = specs.Rasterization;
    const MultisampleState &m = specs.Multisample;
    const DepthStencilState &d = specs.DepthStencil;
    builder.SetTopology(ToVulkan(specs.Topology))
        .EnableDepthBias(r.DepthBiasEnabled)
        .SetDepthBias(r.DepthBiasConstantFactor, r.DepthBiasClamp, r.DepthBiasSlopeFactor)
        .SetLineWidth(r.LineWidth)
        .SetPolygonMode(ToVulkan(r.PolygonMode))
        .SetCullMode(ToVulkan(r.CullMode))
        .SetFrontFace(ToVulkan(r.FrontFace))
        .EnableSampleShading(m.SampleShadingEnabled)
        .EnableAlphaToCoverage(m.AlphaToCoverageEnabled)
        .EnableAlphaToOne(m.AlphaToOneEnabled)
        .SetMinSampleShading(m.MinSampleShading)
        .SetSampleCount(ToVulkan(m.Samples))
        .EnableDepthBoundsTest(d.DepthBoundsTestEnabled)
        .EnableDepthTest(d.DepthTestEnabled)
        .EnableDepthWrite(d.DepthWriteEnabled)
        .EnableStencilTest(d.StencilTestEnabled)
        .SetStencilFailOperation(ToVulkan(d.Front.FailOp), VKit::StencilOperationFlag_Front)
        .SetStencilPassOperation(ToVulkan(d.Front.PassOp), VKit::StencilOperationFlag_Front)
        .SetStencilDepthFailOperation(ToVulkan(d.Front.DepthFailOp), VKit::StencilOperationFlag_Front)
        .SetStencilCompareOperation(ToVulkan(d.Front.CompareOp), VKit::StencilOperationFlag_Front)
        .SetStencilCompareMask(d.Front.CompareMask, VKit::StencilOperationFlag_Front)
        .SetStencilWriteMask(d.Front.WriteMask, VKit::StencilOperationFlag_Front)
        .SetStencilReference(d.Front.Reference, VKit::StencilOperationFlag_Front)
        .SetStencilFailOperation(ToVulkan(d.Back.FailOp), VKit::StencilOperationFlag_Back)
        .SetStencilPassOperation(ToVulkan(d.Back.PassOp), VKit::StencilOperationFlag_Back)
        .SetStencilDepthFailOperation(ToVulkan(d.Back.DepthFailOp), VKit::StencilOperationFlag_Back)
        .SetStencilCompareOperation(ToVulkan(d.Back.CompareOp), VKit::StencilOperationFlag_Back)
        .SetStencilCompareMask(d.Back.CompareMask, VKit::StencilOperationFlag_Back)
        .SetStencilWriteMask(d.Back.WriteMask, VKit::StencilOperationFlag_Back)
        .SetStencilReference(d.Back.Reference, VKit::StencilOperationFlag_Back)
        .SetDepthCompareOperation(ToVulkan(d.DepthCompareOp))
        .SetDepthBounds(d.MinDepthBounds, d.MaxDepthBounds);

    if (specs.DynamicState & DynamicStateFlag_Viewport)
        builder.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
    if (specs.DynamicState & DynamicStateFlag_Scissor)
        builder.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR);
    if (specs.DynamicState & DynamicStateFlag_CullMode)
        builder.AddDynamicState(VK_DYNAMIC_STATE_CULL_MODE_EXT);

    const VKit::GraphicsPipeline pip = GRAPH_CHECK_VKIT_RESULT(builder.Bake().Build());
    const Id id = s_Pipelines->Insert();
    Vulkan_Pipeline &vpip = s_Pipelines->At(id);

    vpip.Pipeline.Construct<VKit::GraphicsPipeline>(pip);
    vpip.Type = Pipeline_Graphics;

    return Handle_Create(Handle_Pipeline, id);
}

Pipeline Pipeline_CreateCompute(const PipelineLayout layout, const ShaderStageInfo &shaderStage)
{
    VKit::ComputePipelineSpecs spc{};
    spc.ComputeShader = GetShader(shaderStage.Shader);
    spc.EntryPoint = shaderStage.EntryPoint;
    spc.Layout = GetPipelineLayout(layout);
    const VKit::ComputePipeline pip = GRAPH_CHECK_VKIT_RESULT(VKit::ComputePipeline::Create(GetDevice(), spc));

    const Id id = s_Pipelines->Insert();
    Vulkan_Pipeline &vpip = s_Pipelines->At(id);

    vpip.Pipeline.Construct<VKit::ComputePipeline>(pip);
    vpip.Type = Pipeline_Compute;

    return Handle_Create(Handle_Pipeline, id);
}

void Pipeline_Destroy(const Pipeline pip)
{
    GRAPH_CHECK_HANDLE(pip, Handle_Pipeline);
    GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(s_Pipelines, pip, pipeline_Destroy);
}

PipelineType Pipeline_GetType(const Pipeline pip)
{
    GRAPH_CHECK_HANDLE(pip, Handle_Pipeline);
    return s_Pipelines->At(Handle_GetId(pip)).Type;
}

void Pipeline_SetName(const Pipeline pip, const char *name)
{
    GRAPH_CHECK_HANDLE(pip, Handle_Pipeline);

    Vulkan_Pipeline &vpip = s_Pipelines->At(Handle_GetId(pip));
    pipeline_Visit(vpip, [name](auto &p) { GRAPH_CHECK_VKIT_RESULT(p.SetName(name)); });
}

bool Pipeline_IsHandleValid(const Pipeline pip)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Pipelines, pip, Handle_Pipeline);
}

VKit::PipelineLayout &GetPipelineLayout(const PipelineLayout layout)
{
    GRAPH_CHECK_HANDLE(layout, Handle_PipelineLayout);
    return s_Layouts->At(Handle_GetId(layout));
}

void BindPipeline(const VkCommandBuffer cmd, const Pipeline pip)
{
    GRAPH_CHECK_HANDLE(pip, Handle_Pipeline);
    pipeline_Visit(s_Pipelines->At(Handle_GetId(pip)), [cmd](const auto &p) { p.Bind(cmd); });
}

} // namespace Graph
