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

} // namespace Graph
