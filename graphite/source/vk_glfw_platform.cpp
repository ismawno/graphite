#include "pch.hpp"
#define GLFW_INCLUDE_VULKAN
#include "glfw_core.hpp"
#include "vk_platform.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/vulkan/loader.hpp"

namespace Graph
{
void Platform_InitializeVulkanLoader()
{
#if GRAPH_GLFW_VERSION_COMBINED >= 3400
    glfwInitVulkanLoader(VKit::Vulkan::vkGetInstanceProcAddr);
#endif
}
VkSurfaceKHR Platform_CreateSurface(const Window win)
{
    VkSurfaceKHR surf;
    const auto &instance = GetInstance();
    GRAPH_CHECK_VKIT_RESULT(
        glfwCreateWindowSurface(instance, GetWindow(win), instance.GetInfo().AllocationCallbacks, &surf));
    return surf;
}

void Platform_DestroySurface(const VkSurfaceKHR surf)
{
    const auto &instance = GetInstance();
    const auto table = GetInstanceTable();
    table->DestroySurfaceKHR(instance, surf, instance.GetInfo().AllocationCallbacks);
}
} // namespace Graph
