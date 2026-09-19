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
    Window Window = nullptr;
};

static TKit::Storage<TKit::ArenaHive<VkSurfaceKHR>> s_Surfaces{};
static DummySurface s_Dummy{};

void Surface_Initialize()
{
    s_Surfaces.Construct();
}
void Surface_Terminate()
{
    TKIT_LOG_WARNING_IF(!s_Surfaces->IsEmpty(), "[GRAPH][SURFACE] {} surfaces have not been freed. Cleaning up...",
                        s_Surfaces->GetSize());
    for (const VkSurfaceKHR surf : *s_Surfaces)
        Platform_DestroySurface(surf);

    s_Surfaces.Destruct();
}

Surface Surface_Create(const Window win)
{
    return Handle_Create(Handle_Surface, s_Surfaces->Insert(Platform_CreateSurface(win)));
}

void Surface_Destroy(const Surface surf)
{
    GRAPH_CHECK_HANDLE(surf, Handle_Surface);
    const auto &instance = GetInstance();
    const auto table = GetInstanceTable();
    table->DestroySurfaceKHR(instance, s_Surfaces->At(Handle_GetId(surf)), instance.GetInfo().AllocationCallbacks);
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
    TKIT_ASSERT(!s_Dummy.Window && !s_Dummy.Surface,
                "[GRAPH][SURFACE] Can only create a single dummy surface at the same time");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    const Window eduardo = glfwCreateWindow(120, 120, "Eduardo", nullptr, nullptr);

    s_Dummy.Window = eduardo;
    s_Dummy.Surface = Platform_CreateSurface(eduardo);
    return s_Dummy.Surface;
}
} // namespace Graph
