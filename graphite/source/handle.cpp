#include "pch.hpp"
#ifdef GRAPH_HAS_PLATFORM_BACKEND
#    include "graph/surface.hpp"
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
    case Handle_Surface:
        return Surface_IsHandleValid(handle);
#endif
    case Handle_Queue:
        return Queue_IsHandleValid(handle);
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
    case Handle_Shader:
        return Shader_IsHandleValid(handle);
#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
    case Handle_Compilation:
        return Compilation_IsHandleValid(handle);
#endif
    default:
        return false;
    }
}
} // namespace Graph
