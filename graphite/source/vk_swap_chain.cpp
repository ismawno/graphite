#include "pch.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "graph/swap_chain.hpp"
#include "graph/execution.hpp"
#include "graph/platform.hpp"
#include "vkit/presentation/swap_chain.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
struct SwapData
{
    Image Image;
    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderFinishedSemaphore;
    Tracker RenderTracker{};
};

struct Vulkan_Swapchain
{
    VKit::Swapchain Swapchain{};
    TKit::TierArray<SwapData> Swap{};
    u32 ImageIndex = 0;
    u32 SyncIndex = 0;
    Format Format;
};

static TKit::Storage<TKit::ArenaHive<Vulkan_Swapchain>> s_Swapchains{};
static VKit::Queue *s_PresentQueue;

static void swapChain_Destroy(Vulkan_Swapchain &sc)
{
    const auto &device = GetDevice();
    const auto &instance = GetInstance();
    const auto table = GetDeviceTable();

    for (const SwapData &sdata : sc.Swap)
        Queue_WaitForTracker(sdata.RenderTracker);

    GRAPH_CHECK_RESULT(s_PresentQueue->WaitIdle());

    for (const SwapData &sdata : sc.Swap)
    {
        Image_Destroy(sdata.Image);
        table->DestroySemaphore(device, sdata.ImageAvailableSemaphore, instance.GetInfo().AllocationCallbacks);
        table->DestroySemaphore(device, sdata.RenderFinishedSemaphore, instance.GetInfo().AllocationCallbacks);
    }

    sc.Swapchain.Destroy();
}

void Swapchain_Initialize(const u32 maxSwapchains)
{
    s_Swapchains.Construct();
    s_Swapchains->Reserve(maxSwapchains);

    const auto &perType = GetDevice().GetInfo().QueuesPerType;
    s_PresentQueue = perType[VKit::Queue_Present].GetFront();
}

void Swapchain_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(s_Swapchains, "SWAP-CHAIN", "swap chains", swapChain_Destroy);

    s_Swapchains.Destruct();
    s_PresentQueue = nullptr;
}

Swapchain Swapchain_Create(const SwapchainSpecs &specs)
{
    const u32v2 extent = Window_GetPixelDimensions(specs.Window);
    TKIT_ASSERT(extent[0] != 0 && extent[1] != 0,
                "[GRAPH][SWAP-CHAIN] Pixel dimensions of window are zero, meaning the window is likely minimized. Wait "
                "until that is not the case before creating a swap chain");

    const auto &instance = GetInstance();
    const auto &device = GetDevice();
    const auto table = GetDeviceTable();
    VKit::Swapchain::Builder builder{&device, GetSurface(specs.Window)};
    builder.RequestSurfaceFormat({ToVulkan(specs.SurfaceFormat), VK_COLORSPACE_SRGB_NONLINEAR_KHR})
        .RequestPresentMode(ToVulkan(specs.PresentMode))
        .RequestImageCount(specs.PreferredImageCount)
        .RequestExtent({extent[0], extent[1]})
        .AddFlags(VKit::SwapchainBuilderFlag_Clipped | VKit::SwapchainBuilderFlag_CreateImageViews)
        .SetOldSwapchain(specs.OldSwapchain == NullHandle
                             ? VK_NULL_HANDLE
                             : s_Swapchains->At(Handle_GetId(specs.OldSwapchain)).Swapchain.GetHandle());

    const Id id = s_Swapchains->Insert();
    Vulkan_Swapchain &sc = s_Swapchains->At(id);
    sc.Swapchain = GRAPH_CHECK_RESULT(builder.Build());
    sc.Format = specs.SurfaceFormat;

    const u32 icount = sc.Swapchain.GetImageCount();
    sc.Swap.Reserve(icount);
    for (u32 i = 0; i < icount; ++i)
    {
        SwapData &sdata = sc.Swap.Append();
        sdata.Image = CreateNonOwnedImage(sc.Swapchain.GetImage(i));

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        GRAPH_CHECK_RESULT(table->CreateSemaphore(device, &semaphoreInfo, instance.GetInfo().AllocationCallbacks,
                                                  &sdata.ImageAvailableSemaphore));
        GRAPH_CHECK_RESULT(table->CreateSemaphore(device, &semaphoreInfo, instance.GetInfo().AllocationCallbacks,
                                                  &sdata.RenderFinishedSemaphore));

        if (IsDebugUtilsEnabled())
        {
            const char *title = Window_GetTitle(specs.Window);
            const TKit::StackString rfinish =
                TKit::StackString::Format("graph-render-finished-semaphore-window-'{}'-image-index-{}", title, i);
            const TKit::StackString iavail =
                TKit::StackString::Format("graph-image-available-semaphore-index-{}-window-'{}'", i, title);

            GRAPH_CHECK_RESULT(
                device.SetObjectName(sdata.RenderFinishedSemaphore, VK_OBJECT_TYPE_SEMAPHORE, rfinish.CString()));
            GRAPH_CHECK_RESULT(
                device.SetObjectName(sdata.ImageAvailableSemaphore, VK_OBJECT_TYPE_SEMAPHORE, iavail.CString()));
            const TKit::StackString pres =
                TKit::StackString::Format("graph-presentation-image-index-{}-window-'{}'", i, title);

            Image_SetName(sdata.Image, pres.CString());
        }
    }

    return Handle_Create(Handle_Swapchain, id);
}

void Swapchain_Destroy(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(s_Swapchains, sc, swapChain_Destroy);
}

Format Swapchain_GetFormat(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    return s_Swapchains->At(Handle_GetId(sc)).Format;
}
u32 Swapchain_GetImageCount(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    return s_Swapchains->At(Handle_GetId(sc)).Swap.GetSize();
}
u32v2 Swapchain_GetExtent(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    const VkExtent2D &ext = s_Swapchains->At(Handle_GetId(sc)).Swapchain.GetInfo().Extent;
    return {ext.width, ext.height};
}
Image Swapchain_GetImage(const Swapchain sc, const u32 imgIdx)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    return s_Swapchains->At(Handle_GetId(sc)).Swap[imgIdx].Image;
}

SwapchainStatus Swapchain_AcquireNextImage(const Swapchain sc, const u64 timeout)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);

    Vulkan_Swapchain &vsc = s_Swapchains->At(Handle_GetId(sc));
    const u32 idx = (vsc.SyncIndex + 1) % vsc.Swap.GetSize();
    const SwapData &swap = vsc.Swap[idx];

    if (!Queue_WaitForTracker(swap.RenderTracker, timeout))
        return SwapchainStatus_NotReady;

    const VkResult res = GetDeviceTable()->AcquireNextImageKHR(
        GetDevice(), vsc.Swapchain, timeout, swap.ImageAvailableSemaphore, VK_NULL_HANDLE, &vsc.ImageIndex);

    if (res == VK_NOT_READY || res == VK_TIMEOUT)
        return SwapchainStatus_NotReady;

    if (res == VK_ERROR_SURFACE_LOST_KHR)
        return SwapchainStatus_SurfaceLost;
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
        return SwapchainStatus_OutOfDate;
    if (res == VK_SUBOPTIMAL_KHR)
        return SwapchainStatus_Suboptimal;

    vsc.SyncIndex = idx;
    GRAPH_CHECK_RESULT(res);
    return SwapchainStatus_Success;
}

SwapchainStatus Swapchain_Present(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);

    Vulkan_Swapchain &vsc = s_Swapchains->At(Handle_GetId(sc));
    const SwapData &swap = vsc.Swap[vsc.ImageIndex];
    const VkSwapchainKHR vksc = vsc.Swapchain;

    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &swap.RenderFinishedSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &vksc;
    info.pImageIndices = &vsc.ImageIndex;

    const VkResult res = GetDeviceTable()->QueuePresentKHR(*s_PresentQueue, &info);
    if (res == VK_ERROR_SURFACE_LOST_KHR)
        return SwapchainStatus_SurfaceLost;
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
        return SwapchainStatus_OutOfDate;
    if (res == VK_SUBOPTIMAL_KHR)
        return SwapchainStatus_Suboptimal;

    GRAPH_CHECK_RESULT(res);
    return SwapchainStatus_Success;
}

void Swapchain_SetName(const Swapchain sc, const char *name)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    GRAPH_CHECK_RESULT(s_Swapchains->At(Handle_GetId(sc)).Swapchain.SetName(name));
}

bool Swapchain_IsHandleValid(const Swapchain sc)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Swapchains, sc, Handle_Swapchain);
}

VkSemaphore GetRenderFinishedSemaphore(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    const Vulkan_Swapchain &vsc = s_Swapchains->At(Handle_GetId(sc));
    return vsc.Swap[vsc.ImageIndex].RenderFinishedSemaphore;
}
VkSemaphore GetImageAvailableSemaphore(const Swapchain sc)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    const Vulkan_Swapchain &vsc = s_Swapchains->At(Handle_GetId(sc));
    return vsc.Swap[vsc.SyncIndex].ImageAvailableSemaphore;
}
void SetRenderTimelineTracker(const Swapchain sc, const Tracker &tr)
{
    GRAPH_CHECK_HANDLE(sc, Handle_Swapchain);
    Vulkan_Swapchain &vsc = s_Swapchains->At(Handle_GetId(sc));
    vsc.Swap[vsc.SyncIndex].RenderTracker = tr;
}
} // namespace Graph
