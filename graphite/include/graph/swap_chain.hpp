#pragma once

#ifndef GRAPH_HAS_PLATFORM_BACKEND
#    error                                                                                                             \
        "[GRAPH][SWAP-CHAIN] To use swap chain capabilities, a platform backend must be specified with the CMake option GRAPHITE_PLATFORM_BACKEND"
#endif

#include "graph/resources.hpp"
#include "tkit/math/tensor.hpp"

namespace Graph
{
struct SwapchainSpecs
{
    Window Window = NullHandle;
    Format SurfaceFormat = Format_B8G8R8A8_UNORM;
    PresentMode PresentMode = PresentMode_VSync;
    u32 PreferredImageCount = 3;
    Swapchain OldSwapchain = NullHandle;
};

Swapchain Swapchain_Create(const SwapchainSpecs &specs);
void Swapchain_Destroy(Swapchain sc);

Format Swapchain_GetFormat(Swapchain sc);
u32 Swapchain_GetImageCount(Swapchain sc);
u32v2 Swapchain_GetExtent(Swapchain sc);

u32 Swapchain_GetCurrentIndex(Swapchain sc);
Image Swapchain_GetImage(Swapchain sc, u32 imgIdx);

inline Image Swapchain_GetCurrentImage(const Swapchain sc)
{
    return Swapchain_GetImage(sc, Swapchain_GetCurrentIndex(sc));
}

inline ImageView Swapchain_GetImageView(const Swapchain sc, const u32 imgIdx)
{
    return Image_GetView(sc, Swapchain_GetImage(sc, imgIdx));
}

enum SwapchainStatus : u8
{
    SwapchainStatus_Success,
    SwapchainStatus_Suboptimal,
    SwapchainStatus_OutOfDate,
    SwapchainStatus_SurfaceLost,
    SwapchainStatus_NotReady,
};

SwapchainStatus Swapchain_AcquireNextImage(Swapchain sc, u64 timeout = TKIT_U64_MAX);
SwapchainStatus Swapchain_Present(Swapchain sc);

void Swapchain_SetName(Swapchain sc, const char *name);
bool Swapchain_IsHandleValid(Swapchain sc);
} // namespace Graph
