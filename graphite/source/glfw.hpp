#pragma once

#include <GLFW/glfw3.h>

#define GRAPH_GLFW_VERSION_COMBINED (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 + GLFW_VERSION_REVISION)
#if GRAPH_GLFW_VERSION_COMBINED >= 3200 && !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
#    define GRAPH_GLFW_VULKAN
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_WINDOW_HOVERED // 3.3+ GLFW_HOVERED
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_WINDOW_OPACITY // 3.3+ glfwSetWindowOpacity()
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_PER_MONITOR_DPI // 3.3+ glfwGetMonitorContentScale()
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_FOCUS_ON_SHOW // 3.3+ GLFW_FOCUS_ON_SHOW
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_MONITOR_WORK_AREA // 3.3+ glfwGetMonitorWorkarea()
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3301
#    define GRAPH_GLFW_OSX_WINDOW_POS_FIX // 3.3.1+ Fixed: Resizing window repositions it on MacOS #1553
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3400 && defined(GLFW_RESIZE_NESW_CURSOR)
#    define GRAPH_GLFW_NEW_CURSORS                                                                                     \
        // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR,
        // GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#endif
#if GRAPH_GLFW_VERSION_COMBINED >= 3400 && defined(GLFW_MOUSE_PASSTHROUGH)
#    define GRAPH_GLFW_MOUSE_PASSTHROUGH // 3.4+ GLFW_MOUSE_PASSTHROUGH
#endif

#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_GAMEPAD_API // 3.3+ glfwGetGamepadState() new api
#endif
#if GRAPH_GLFW_VERSION_COMBINED >= 3300
#    define GRAPH_GLFW_GETERROR // 3.3+ glfwGetError()
#endif
#if GRAPH_GLFW_VERSION_COMBINED >= 3400
#    define GRAPH_GLFW_GETPLATFORM // 3.4+ glfwGetPlatform()
#endif

#ifndef GRAPH_GLFW_VULKAN
#    error                                                                                                             \
        "[GRAPH][GLFW] Vulkan is not supported in this version of GLFW. At least 3.2 is required (switch and emscripten are not supported either)"
#endif
