#include "pch.hpp"
#define GLFW_INCLUDE_VULKAN
#include "graph/glfw.hpp"
#include "vk_platform.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/vulkan/loader.hpp"
#include "tkit/utils/debug.hpp"

namespace Graph
{
#ifdef TKIT_ENABLE_ERROR_LOGS
static void glfwErrorCallback(const i32 errorCode, const char *description)
{
    TKIT_LOG_ERROR("[GRAPH][GLFW] An error ocurred with code {} and the following description: {}", errorCode,
                   description);
}
#endif

static u32 getPlatform(const Platform plat)
{
    switch (plat)
    {
    case Platform_Any:
        return GLFW_ANY_PLATFORM;
    case Platform_Win32:
        return GLFW_PLATFORM_WIN32;
    case Platform_Cocoa:
        return GLFW_PLATFORM_COCOA;
    case Platform_Wayland:
        return GLFW_PLATFORM_WAYLAND;
    case Platform_X11:
        return GLFW_PLATFORM_X11;
    }
    return GLFW_ANY_PLATFORM;
}

void Platform_Initialize(const Platform plat)
{
#ifdef TKIT_ENABLE_ERROR_LOGS
    glfwSetErrorCallback(glfwErrorCallback);
#endif
    glfwInitHint(GLFW_PLATFORM, getPlatform(plat));
#if GRAPH_GLFW_VERSION_COMBINED >= 3400
    glfwInitVulkanLoader(VKit::Vulkan::vkGetInstanceProcAddr);
#endif
    TKIT_ENSURE_RETURNS(glfwInit(), GLFW_TRUE, "[GRAPH][PLATFORM] GLFW failed to initialize");

    TKIT_LOG_WARNING_IF(!glfwVulkanSupported(), "[GRAPH][PLATFORM] Vulkan is not supported, according to GLFW");
}
void Platform_Terminate()
{
    glfwTerminate();
}

VkSurfaceKHR Platform_CreateSurface(GLFWwindow *win)
{
    VkSurfaceKHR surf;
    const auto &instance = GetInstance();
    GRAPH_CHECK_VKIT_RESULT(glfwCreateWindowSurface(instance, win, instance.GetInfo().AllocationCallbacks, &surf));
    return surf;
}

void Platform_DestroySurface(const VkSurfaceKHR surf)
{
    const auto &instance = GetInstance();
    const auto table = GetInstanceTable();
    table->DestroySurfaceKHR(instance, surf, instance.GetInfo().AllocationCallbacks);
}
} // namespace Graph
