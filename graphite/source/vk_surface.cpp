#include "pch.hpp"
#include "graph/surface.hpp"
#include "vk_core.hpp"
#include "vk_platform.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"
#include <vulkan/vulkan.h>
#ifdef GRAPH_PLATFORM_BACKEND_GLFW
#    define GLFW_INCLUDE_VULKAN
#    include <GLFW/glfw3.h>
#endif

namespace Graph
{
struct DummySurface
{
    VkSurfaceKHR Surface = VK_NULL_HANDLE;
    Window Window;
};

static TKit::Storage<TKit::ArenaHive<VkSurfaceKHR>> s_Surfaces{};
static DummySurface s_Dummy{};

void Surface_Initialize(const u32 maxSurfaces)
{
    s_Surfaces.Construct();
    s_Surfaces->Reserve(maxSurfaces);
}
void Surface_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(s_Surfaces, "SURFACE", "surfaces", Platform_DestroySurface);
    s_Surfaces.Destruct();
}

Surface Surface_Create(const Window win)
{
    return Handle_Create(Handle_Surface, s_Surfaces->Insert(Platform_CreateSurface(win)));
}

void Surface_Destroy(const Surface surf)
{
    GRAPH_CHECK_HANDLE(surf, Handle_Surface);
    GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(s_Surfaces, surf, Platform_DestroySurface);
}

bool Surface_IsHandleValid(const Surface surf)
{
    if (Handle_GetType(surf) != Handle_Surface)
        return false;
    return s_Surfaces->Contains(Handle_GetId(surf));
}

VkSurfaceKHR GetSurface(const Surface surf)
{
    GRAPH_CHECK_HANDLE(surf, Handle_Surface);
    return s_Surfaces->At(Handle_GetId(surf));
}

VkSurfaceKHR CreateDummySurface()
{
    TKIT_ASSERT(!s_Dummy.Surface, "[GRAPH][SURFACE] Can only create a single dummy surface at the same time");
    const Window eduardo = Window_Create({.Title = "Eduardo", .Dimensions = 120, .Flags = 0});

    s_Dummy.Window = eduardo;
    s_Dummy.Surface = Platform_CreateSurface(eduardo);
    return s_Dummy.Surface;
}

void DestroyDummySurface()
{
    TKIT_ASSERT(s_Dummy.Surface, "[GRAPH][SURFACE] Can only destroy a dummy surface if one was created");

    Platform_DestroySurface(s_Dummy.Surface);
    s_Dummy.Surface = VK_NULL_HANDLE;
}
} // namespace Graph
