#pragma once

#ifndef GRAPH_HAS_PLATFORM_BACKEND
#    error                                                                                                             \
        "[GRAPH][PLATFORM] To use platform capabilities, a platform backend must be specified with the CMake option GRAPHITE_PLATFORM_BACKEND"
#endif

#include "graph/core.hpp"
#include "graph/input.hpp"
#include "tkit/math/tensor.hpp"

namespace Graph
{

struct VideoMode
{
    u32v2 Dimensions;
    u32v3 BitDepths;
    u32 RefreshRate;
};

struct WindowSpecs
{
    const char *Title = "Graphite window";
    i32v2 Position{TKIT_I32_MAX}; // i32 max means let it be decided automatically
    u32v2 Dimensions{800, 600};
    WindowFlags Flags = WindowFlag_Resizable | WindowFlag_Visible | WindowFlag_Decorated | WindowFlag_Focused;
    void *UserData = nullptr;
};

Window Window_Create(const WindowSpecs &specs);
void Window_Destroy(Window win);

void *Window_GetUserData(Window win);
void Window_SetUserData(Window win, void *data);
bool Window_ShouldClose(Window win);

void Window_Show(Window win);
void Window_Hide(Window win);
void Window_Focus(Window win);
const char *Window_GetTitle(Window win);

i32v2 Window_GetPosition(Window win);
u32v2 Window_GetScreenDimensions(Window win);
u32v2 Window_GetPixelDimensions(Window win);
f32v2 Window_GetCursorPosition(Window win);
f32 Window_GetAspect(Window win);
f32 Window_GetOpacity(Window win);
WindowFlags Window_GetFlags(Window win);

void Window_SetTitle(Window win, const char *title);
void Window_SetPosition(Window win, const i32v2 &pos);
void Window_SetScreenDimensions(Window win, const u32v2 &dim);
void Window_SetAspect(Window win, u32 numer, u32 denom);
void Window_SetFlags(Window win, WindowFlags flags);
void Window_AddFlags(Window win, WindowFlags flags);
void Window_RemoveFlags(Window win, WindowFlags flags);
void Window_SetOpacity(Window win, f32 opacity);

void Window_SetMouseCursor(Window win, MouseCursor cursor);

bool Window_IsKeyPressed(Window win, Key key);
bool Window_IsKeyReleased(Window win, Key key);

bool Window_IsMousePressed(Window win, Mouse button);
bool Window_IsMouseReleased(Window win, Mouse button);

void Window_SignalShouldClose();
Monitor Window_GetMonitor(Window win);

bool Window_IsHandleValid(Window win);

Monitor Monitor_GetPrimary();

void *Monitor_GetUserData(Monitor monitor);
void Monitor_SetUserData(Monitor monitor, void *data);

i32v2 Monitor_GetPosition(Monitor monitor);
VideoMode Monitor_GetVideoMode(Monitor monitor);

bool Monitor_IsHandleValid(Monitor monitor);

enum InputAction : u8
{
    InputAction_Press,
    InputAction_Release,
    InputAction_Repeat,
};

using KeyModFlags = u8;
enum KeyModFlagBit : u8
{
    KeyModFlag_Shift = 1U << 0,
    KeyModFlag_Control = 1U << 1,
    KeyModFlag_Alt = 1U << 2,
    KeyModFlag_Super = 1U << 3,
};

using WindowPosCallback = void (*)(Window win, i32 x, i32 y);
using WindowSizeCallback = void (*)(Window win, u32 width, u32 height);
using FramebufferSizeCallback = void (*)(Window win, u32 width, u32 height);
using WindowFocusCallback = void (*)(Window win, bool focused);
using WindowCloseCallback = void (*)(Window win);
using WindowIconifyCallback = void (*)(Window win, bool iconified);

using KeyCallback = void (*)(Window win, Key key, i32 scancode, InputAction action, KeyModFlags mods);
using CharCallback = void (*)(Window win, u32 codepoint);

using CursorPosCallback = void (*)(Window win, f64 x, f64 y);
using CursorEnterCallback = void (*)(Window win, bool entered);
using MouseButtonCallback = void (*)(Window win, Mouse button, InputAction action, KeyModFlags mods);
using ScrollCallback = void (*)(Window win, f64 xoffset, f64 yoffset);

WindowPosCallback Window_Callback_Position(Window win, WindowPosCallback callback);
WindowSizeCallback Window_Callback_Size(Window win, WindowSizeCallback callback);
FramebufferSizeCallback Window_Callback_FramebufferSize(Window win, FramebufferSizeCallback callback);
WindowFocusCallback Window_Callback_Focus(Window win, WindowFocusCallback callback);
WindowCloseCallback Window_Callback_Close(Window win, WindowCloseCallback callback);
WindowIconifyCallback Window_Callback_Iconify(Window win, WindowIconifyCallback callback);

KeyCallback Window_Callback_Key(Window win, KeyCallback callback);
CharCallback Window_Callback_Char(Window win, CharCallback callback);

CursorPosCallback Window_Callback_CursorPos(Window win, CursorPosCallback callback);
CursorEnterCallback Window_Callback_CursorEnter(Window win, CursorEnterCallback callback);
MouseButtonCallback Window_Callback_MouseButton(Window win, MouseButtonCallback callback);
ScrollCallback Window_Callback_Scroll(Window win, ScrollCallback callback);

} // namespace Graph
