#pragma once

#include "graph/alias.hpp"
#include "graph/platform.hpp"
#include "tkit/memory/memory.hpp"

#ifdef GRAPH_PLATFORM_BACKEND_GLFW
struct GLFWwindow;
namespace Graph
{
using Window = GLFWwindow *;
}
#else
namespace Graph
{
using Window = void *;
}
#endif

namespace Graph
{
struct Allocation
{
    TKit::ArenaAllocator *Arena = nullptr;
    TKit::StackAllocator *Stack = nullptr;
    TKit::TierAllocator *Tier = nullptr;
};

using Capabilities = u16;
enum CapabilityFlagBit : Capabilities
{
    Capability_Validation = 1U << 0,
    Capability_DebugPrintf = 1U << 1,
    Capability_FaultDump = 1U << 2,
    Capability_IndependentBlend = 1U << 3,
    Capability_MultiDrawIndirect = 1U << 4,
    Capability_ShaderDrawParameters = 1U << 5,
    Capability_ExtendedDynamicState = 1U << 6,
    Capability_BindlessDescriptors = 1U << 7,
    Capability_DynamicRendering = 1U << 8,
    Capability_TimelineSemaphores = 1U << 9,
    Capability_ImageMultiFormat = 1U << 10,
};

struct Specs
{
    const char *ApplicationName = "Graphite app";
    const char *LoaderPath = nullptr;
    const char *DumpPath = nullptr;
    Allocation Allocators{};
    Capabilities EnabledCapabilities = 0;
    Platform TargetPlatform = Platform_Auto;
};

void Initialize(const Specs &specs);
void Terminate();

} // namespace Graph
