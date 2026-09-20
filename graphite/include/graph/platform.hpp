#pragma once

#ifndef GRAPH_HAS_PLATFORM_BACKEND
#    error                                                                                                             \
        "[GRAPH][PLATFORM] To use platform capabilities, a platform backend must be specified with the CMake option GRAPHITE_PLATFORM_BACKEND"
#endif

#include "graph/alias.hpp"

namespace Graph
{
enum Platform : u8
{
    Platform_Any,
    Platform_Win32,
    Platform_Cocoa,
    Platform_Wayland,
    Platform_X11,
#ifdef TKIT_OS_LINUX
    Platform_Auto = Platform_X11,
#elif defined(TKIT_OS_APPLE)
    Platform_Auto = Platform_Cocoa,
#elif defined(TKIT_OS_WINDOWS)
    Platform_Auto = Platform_Win32,
#else
    Platform_Auto = Platform_Any,
#endif
};
} // namespace Graph
