#include "pch.hpp"
#include "graph/core.hpp"
#ifdef GRAPH_HAS_PLATFORM_BACKEND
#    include "graph/platform.hpp"
#    include "graph/swap_chain.hpp"
#endif
#include "graph/execution.hpp"
#include "graph/resources.hpp"
#include "graph/shader.hpp"

namespace Graph
{
bool Handle_IsValid(const Handle handle, const HandleType htype)
{
    const HandleType itype = Handle_GetType(handle);
    if (itype >= Handle_Count || (itype != htype && htype != Handle_None))
        return false;

    switch (itype)
    {
#ifdef GRAPH_HAS_PLATFORM_BACKEND
    case Handle_Window:
        return Window_IsHandleValid(handle);
    case Handle_Monitor:
        return Monitor_IsHandleValid(handle);
    case Handle_Swapchain:
        return Swapchain_IsHandleValid(handle);
#endif
    case Handle_CommandPool:
        return CommandPool_IsHandleValid(handle);
    case Handle_CommandBuffer:
        return CommandBuffer_IsHandleValid(handle);
    case Handle_Buffer:
        return Buffer_IsHandleValid(handle);
    case Handle_Image:
        return Image_IsHandleValid(handle);
    case Handle_ImageView:
        return ImageView_IsHandleValid(handle);
    case Handle_Sampler:
        return Sampler_IsHandleValid(handle);
    case Handle_DescriptorSet:
        return DescriptorSet_IsHandleValid(handle);
    case Handle_DescriptorSetLayout:
        return DescriptorSetLayout_IsHandleValid(handle);
    case Handle_Shader:
        return Shader_IsHandleValid(handle);
#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
    case Handle_Compilation:
        return Compilation_IsHandleValid(handle);
#endif
#ifdef GRAPH_HAS_SHADER_REFLECTION_BACKEND
    case Handle_Reflection:
        return Reflection_IsHandleValid(handle);
#endif
    case Handle_PipelineLayout:
        return PipelineLayout_IsHandleValid(handle);
    case Handle_Pipeline:
        return Pipeline_IsHandleValid(handle);
    default:
        return false;
    }
}

const char *ToString(const HandleType htype)
{
    switch (htype)
    {
    case Handle_Window:
        return "Handle_Window";
    case Handle_Monitor:
        return "Handle_Monitor";
    case Handle_Swapchain:
        return "Handle_Swapchain";
    case Handle_CommandPool:
        return "Handle_CommandPool";
    case Handle_CommandBuffer:
        return "Handle_CommandBuffer";
    case Handle_Buffer:
        return "Handle_Buffer";
    case Handle_Image:
        return "Handle_Image";
    case Handle_ImageView:
        return "Handle_ImageView";
    case Handle_Sampler:
        return "Handle_Sampler";
    case Handle_DescriptorSetLayout:
        return "Handle_DescriptorSetLayout";
    case Handle_DescriptorSet:
        return "Handle_DescriptorSet";
    case Handle_Shader:
        return "Handle_Shader";
    case Handle_Compilation:
        return "Handle_Compilation";
    case Handle_Reflection:
        return "Handle_Reflection";
    case Handle_PipelineLayout:
        return "Handle_PipelineLayout";
    case Handle_Pipeline:
        return "Handle_Pipeline";
    case Handle_None:
        return "Handle_None";
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown handle type: {}", u32(htype));
        return "Handle_Unknown";
    }
}
} // namespace Graph
