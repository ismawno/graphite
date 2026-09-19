#include "pch.hpp"
#include "graph/surface.hpp"

namespace Graph
{
bool Handle_IsValid(const Handle handle, const HandleType htype)
{
    const HandleType itype = Handle_GetType(handle);
    if (itype >= Handle_Count || (itype != htype && htype != Handle_None))
        return false;

    switch (itype)
    {
    case Handle_Surface:
        return Surface_IsHandleValid(handle);
    default:
        return false;
    }
}
} // namespace Graph
