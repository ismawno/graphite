#include "pch.hpp"
#include "glfw_core.hpp"
#include "graph/platform.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
struct Glfw_Window
{
    GLFWwindow *Window;
    void *UserData;
    TKit::FixedArray<GLFWcursor *, MouseCursor_Count> Cursors{};
    Graph::Window Handle;

    WindowPosCallback PositionCallback = nullptr;
    WindowSizeCallback SizeCallback = nullptr;
    FramebufferSizeCallback FramebufferSizeCallback = nullptr;
    WindowFocusCallback FocusCallback = nullptr;
    WindowCloseCallback CloseCallback = nullptr;
    WindowIconifyCallback IconifyCallback = nullptr;
    KeyCallback KeyCallback = nullptr;
    CharCallback CharCallback = nullptr;
    CursorPosCallback CursorPosCallback = nullptr;
    CursorEnterCallback CursorEnterCallback = nullptr;
    MouseButtonCallback MouseButtonCallback = nullptr;
    ScrollCallback ScrollCallback = nullptr;
};

struct Glfw_Monitor
{
    GLFWmonitor *Monitor;
    void *UserData;
    Graph::Monitor Handle;
};

static TKit::Storage<TKit::ArenaHive<Glfw_Window>> s_Windows{};
static TKit::Storage<TKit::ArenaArray<Glfw_Monitor>> s_Monitors{};

#ifdef TKIT_ENABLE_ERROR_LOGS
static void glfwErrorCallback(const i32 errorCode, const char *description)
{
    TKIT_LOG_ERROR("[GRAPH][GLFW] An error ocurred with code {} and the following description: {}", errorCode,
                   description);
}
#endif

static u32 toGlfw(const Platform plat)
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

static u32 toGlfw(const MouseCursor cursor)
{
    switch (cursor)
    {
    case MouseCursor_Arrow:
        return GLFW_ARROW_CURSOR;
    case MouseCursor_NS:
        return GLFW_RESIZE_NS_CURSOR;
    case MouseCursor_EW:
        return GLFW_RESIZE_EW_CURSOR;
    case MouseCursor_NESW:
        return GLFW_RESIZE_NESW_CURSOR;
    case MouseCursor_NWSE:
        return GLFW_RESIZE_NWSE_CURSOR;
    case MouseCursor_Hand:
        return GLFW_POINTING_HAND_CURSOR;
    case MouseCursor_CrossHair:
        return GLFW_CROSSHAIR_CURSOR;
    case MouseCursor_IBeam:
        return GLFW_IBEAM_CURSOR;
    case MouseCursor_NotAllowed:
        return GLFW_NOT_ALLOWED_CURSOR;
    default:
        TKIT_FATAL("[GRAPH][WINDOW] Failed to find cursors");
        return TKIT_U32_MAX;
    }
}
static i32 toGlfw(const Key key)
{
    switch (key)
    {
    case Key_Space:
        return GLFW_KEY_SPACE;
    case Key_Apostrophe:
        return GLFW_KEY_APOSTROPHE;
    case Key_Comma:
        return GLFW_KEY_COMMA;
    case Key_Minus:
        return GLFW_KEY_MINUS;
    case Key_Period:
        return GLFW_KEY_PERIOD;
    case Key_Slash:
        return GLFW_KEY_SLASH;
    case Key_N0:
        return GLFW_KEY_0;
    case Key_N1:
        return GLFW_KEY_1;
    case Key_N2:
        return GLFW_KEY_2;
    case Key_N3:
        return GLFW_KEY_3;
    case Key_N4:
        return GLFW_KEY_4;
    case Key_N5:
        return GLFW_KEY_5;
    case Key_N6:
        return GLFW_KEY_6;
    case Key_N7:
        return GLFW_KEY_7;
    case Key_N8:
        return GLFW_KEY_8;
    case Key_N9:
        return GLFW_KEY_9;
    case Key_Semicolon:
        return GLFW_KEY_SEMICOLON;
    case Key_Equal:
        return GLFW_KEY_EQUAL;
    case Key_A:
        return GLFW_KEY_A;
    case Key_B:
        return GLFW_KEY_B;
    case Key_C:
        return GLFW_KEY_C;
    case Key_D:
        return GLFW_KEY_D;
    case Key_E:
        return GLFW_KEY_E;
    case Key_F:
        return GLFW_KEY_F;
    case Key_G:
        return GLFW_KEY_G;
    case Key_H:
        return GLFW_KEY_H;
    case Key_I:
        return GLFW_KEY_I;
    case Key_J:
        return GLFW_KEY_J;
    case Key_K:
        return GLFW_KEY_K;
    case Key_L:
        return GLFW_KEY_L;
    case Key_M:
        return GLFW_KEY_M;
    case Key_N:
        return GLFW_KEY_N;
    case Key_O:
        return GLFW_KEY_O;
    case Key_P:
        return GLFW_KEY_P;
    case Key_Q:
        return GLFW_KEY_Q;
    case Key_R:
        return GLFW_KEY_R;
    case Key_S:
        return GLFW_KEY_S;
    case Key_T:
        return GLFW_KEY_T;
    case Key_U:
        return GLFW_KEY_U;
    case Key_V:
        return GLFW_KEY_V;
    case Key_W:
        return GLFW_KEY_W;
    case Key_X:
        return GLFW_KEY_X;
    case Key_Y:
        return GLFW_KEY_Y;
    case Key_Z:
        return GLFW_KEY_Z;
    case Key_LeftBracket:
        return GLFW_KEY_LEFT_BRACKET;
    case Key_Backslash:
        return GLFW_KEY_BACKSLASH;
    case Key_RightBracket:
        return GLFW_KEY_RIGHT_BRACKET;
    case Key_GraveAccent:
        return GLFW_KEY_GRAVE_ACCENT;
    case Key_World_1:
        return GLFW_KEY_WORLD_1;
    case Key_World_2:
        return GLFW_KEY_WORLD_2;
    case Key_Escape:
        return GLFW_KEY_ESCAPE;
    case Key_Enter:
        return GLFW_KEY_ENTER;
    case Key_Tab:
        return GLFW_KEY_TAB;
    case Key_Backspace:
        return GLFW_KEY_BACKSPACE;
    case Key_Insert:
        return GLFW_KEY_INSERT;
    case Key_Delete:
        return GLFW_KEY_DELETE;
    case Key_Right:
        return GLFW_KEY_RIGHT;
    case Key_Left:
        return GLFW_KEY_LEFT;
    case Key_Down:
        return GLFW_KEY_DOWN;
    case Key_Up:
        return GLFW_KEY_UP;
    case Key_PageUp:
        return GLFW_KEY_PAGE_UP;
    case Key_PageDown:
        return GLFW_KEY_PAGE_DOWN;
    case Key_Home:
        return GLFW_KEY_HOME;
    case Key_End:
        return GLFW_KEY_END;
    case Key_CapsLock:
        return GLFW_KEY_CAPS_LOCK;
    case Key_ScrollLock:
        return GLFW_KEY_SCROLL_LOCK;
    case Key_NumLock:
        return GLFW_KEY_NUM_LOCK;
    case Key_PrintScreen:
        return GLFW_KEY_PRINT_SCREEN;
    case Key_Pause:
        return GLFW_KEY_PAUSE;
    case Key_F1:
        return GLFW_KEY_F1;
    case Key_F2:
        return GLFW_KEY_F2;
    case Key_F3:
        return GLFW_KEY_F3;
    case Key_F4:
        return GLFW_KEY_F4;
    case Key_F5:
        return GLFW_KEY_F5;
    case Key_F6:
        return GLFW_KEY_F6;
    case Key_F7:
        return GLFW_KEY_F7;
    case Key_F8:
        return GLFW_KEY_F8;
    case Key_F9:
        return GLFW_KEY_F9;
    case Key_F10:
        return GLFW_KEY_F10;
    case Key_F11:
        return GLFW_KEY_F11;
    case Key_F12:
        return GLFW_KEY_F12;
    case Key_F13:
        return GLFW_KEY_F13;
    case Key_F14:
        return GLFW_KEY_F14;
    case Key_F15:
        return GLFW_KEY_F15;
    case Key_F16:
        return GLFW_KEY_F16;
    case Key_F17:
        return GLFW_KEY_F17;
    case Key_F18:
        return GLFW_KEY_F18;
    case Key_F19:
        return GLFW_KEY_F19;
    case Key_F20:
        return GLFW_KEY_F20;
    case Key_F21:
        return GLFW_KEY_F21;
    case Key_F22:
        return GLFW_KEY_F22;
    case Key_F23:
        return GLFW_KEY_F23;
    case Key_F24:
        return GLFW_KEY_F24;
    case Key_F25:
        return GLFW_KEY_F25;
    case Key_KP_0:
        return GLFW_KEY_KP_0;
    case Key_KP_1:
        return GLFW_KEY_KP_1;
    case Key_KP_2:
        return GLFW_KEY_KP_2;
    case Key_KP_3:
        return GLFW_KEY_KP_3;
    case Key_KP_4:
        return GLFW_KEY_KP_4;
    case Key_KP_5:
        return GLFW_KEY_KP_5;
    case Key_KP_6:
        return GLFW_KEY_KP_6;
    case Key_KP_7:
        return GLFW_KEY_KP_7;
    case Key_KP_8:
        return GLFW_KEY_KP_8;
    case Key_KP_9:
        return GLFW_KEY_KP_9;
    case Key_KPDecimal:
        return GLFW_KEY_KP_DECIMAL;
    case Key_KPDivide:
        return GLFW_KEY_KP_DIVIDE;
    case Key_KPMultiply:
        return GLFW_KEY_KP_MULTIPLY;
    case Key_KPSubtract:
        return GLFW_KEY_KP_SUBTRACT;
    case Key_KPAdd:
        return GLFW_KEY_KP_ADD;
    case Key_KPEnter:
        return GLFW_KEY_KP_ENTER;
    case Key_KPEqual:
        return GLFW_KEY_KP_EQUAL;
    case Key_LeftShift:
        return GLFW_KEY_LEFT_SHIFT;
    case Key_LeftControl:
        return GLFW_KEY_LEFT_CONTROL;
    case Key_LeftAlt:
        return GLFW_KEY_LEFT_ALT;
    case Key_LeftSuper:
        return GLFW_KEY_LEFT_SUPER;
    case Key_RightShift:
        return GLFW_KEY_RIGHT_SHIFT;
    case Key_RightControl:
        return GLFW_KEY_RIGHT_CONTROL;
    case Key_RightAlt:
        return GLFW_KEY_RIGHT_ALT;
    case Key_RightSuper:
        return GLFW_KEY_RIGHT_SUPER;
    case Key_Menu:
        return GLFW_KEY_MENU;
    case Key_None:
        return GLFW_KEY_LAST + 1;
    case Key_Count:
        return GLFW_KEY_LAST + 1;
    }
    return GLFW_KEY_LAST + 1;
}

static i32 toGlfw(const Mouse mouse)
{
    switch (mouse)
    {
    case Mouse_Button1:
        return GLFW_MOUSE_BUTTON_1;
    case Mouse_Button2:
        return GLFW_MOUSE_BUTTON_2;
    case Mouse_Button3:
        return GLFW_MOUSE_BUTTON_3;
    case Mouse_Button4:
        return GLFW_MOUSE_BUTTON_4;
    case Mouse_Button5:
        return GLFW_MOUSE_BUTTON_5;
    case Mouse_Button6:
        return GLFW_MOUSE_BUTTON_6;
    case Mouse_Button7:
        return GLFW_MOUSE_BUTTON_7;
    case Mouse_Button8:
        return GLFW_MOUSE_BUTTON_8;
    case Mouse_ButtonLast:
        return GLFW_MOUSE_BUTTON_LAST;
    case Mouse_ButtonLeft:
        return GLFW_MOUSE_BUTTON_LEFT;
    case Mouse_ButtonRight:
        return GLFW_MOUSE_BUTTON_RIGHT;
    case Mouse_ButtonMiddle:
        return GLFW_MOUSE_BUTTON_MIDDLE;
    case Mouse_None:
        return GLFW_MOUSE_BUTTON_LAST + 1;
    case Mouse_Count:
        return GLFW_MOUSE_BUTTON_LAST + 1;
    }
    return GLFW_MOUSE_BUTTON_LAST + 1;
}

static Key toKey(const i32 key)
{
    switch (key)
    {
    case GLFW_KEY_SPACE:
        return Key_Space;
    case GLFW_KEY_APOSTROPHE:
        return Key_Apostrophe;
    case GLFW_KEY_COMMA:
        return Key_Comma;
    case GLFW_KEY_MINUS:
        return Key_Minus;
    case GLFW_KEY_PERIOD:
        return Key_Period;
    case GLFW_KEY_SLASH:
        return Key_Slash;
    case GLFW_KEY_0:
        return Key_N0;
    case GLFW_KEY_1:
        return Key_N1;
    case GLFW_KEY_2:
        return Key_N2;
    case GLFW_KEY_3:
        return Key_N3;
    case GLFW_KEY_4:
        return Key_N4;
    case GLFW_KEY_5:
        return Key_N5;
    case GLFW_KEY_6:
        return Key_N6;
    case GLFW_KEY_7:
        return Key_N7;
    case GLFW_KEY_8:
        return Key_N8;
    case GLFW_KEY_9:
        return Key_N9;
    case GLFW_KEY_SEMICOLON:
        return Key_Semicolon;
    case GLFW_KEY_EQUAL:
        return Key_Equal;
    case GLFW_KEY_A:
        return Key_A;
    case GLFW_KEY_B:
        return Key_B;
    case GLFW_KEY_C:
        return Key_C;
    case GLFW_KEY_D:
        return Key_D;
    case GLFW_KEY_E:
        return Key_E;
    case GLFW_KEY_F:
        return Key_F;
    case GLFW_KEY_G:
        return Key_G;
    case GLFW_KEY_H:
        return Key_H;
    case GLFW_KEY_I:
        return Key_I;
    case GLFW_KEY_J:
        return Key_J;
    case GLFW_KEY_K:
        return Key_K;
    case GLFW_KEY_L:
        return Key_L;
    case GLFW_KEY_M:
        return Key_M;
    case GLFW_KEY_N:
        return Key_N;
    case GLFW_KEY_O:
        return Key_O;
    case GLFW_KEY_P:
        return Key_P;
    case GLFW_KEY_Q:
        return Key_Q;
    case GLFW_KEY_R:
        return Key_R;
    case GLFW_KEY_S:
        return Key_S;
    case GLFW_KEY_T:
        return Key_T;
    case GLFW_KEY_U:
        return Key_U;
    case GLFW_KEY_V:
        return Key_V;
    case GLFW_KEY_W:
        return Key_W;
    case GLFW_KEY_X:
        return Key_X;
    case GLFW_KEY_Y:
        return Key_Y;
    case GLFW_KEY_Z:
        return Key_Z;
    case GLFW_KEY_LEFT_BRACKET:
        return Key_LeftBracket;
    case GLFW_KEY_BACKSLASH:
        return Key_Backslash;
    case GLFW_KEY_RIGHT_BRACKET:
        return Key_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT:
        return Key_GraveAccent;
    case GLFW_KEY_WORLD_1:
        return Key_World_1;
    case GLFW_KEY_WORLD_2:
        return Key_World_2;
    case GLFW_KEY_ESCAPE:
        return Key_Escape;
    case GLFW_KEY_ENTER:
        return Key_Enter;
    case GLFW_KEY_TAB:
        return Key_Tab;
    case GLFW_KEY_BACKSPACE:
        return Key_Backspace;
    case GLFW_KEY_INSERT:
        return Key_Insert;
    case GLFW_KEY_DELETE:
        return Key_Delete;
    case GLFW_KEY_RIGHT:
        return Key_Right;
    case GLFW_KEY_LEFT:
        return Key_Left;
    case GLFW_KEY_DOWN:
        return Key_Down;
    case GLFW_KEY_UP:
        return Key_Up;
    case GLFW_KEY_PAGE_UP:
        return Key_PageUp;
    case GLFW_KEY_PAGE_DOWN:
        return Key_PageDown;
    case GLFW_KEY_HOME:
        return Key_Home;
    case GLFW_KEY_END:
        return Key_End;
    case GLFW_KEY_CAPS_LOCK:
        return Key_CapsLock;
    case GLFW_KEY_SCROLL_LOCK:
        return Key_ScrollLock;
    case GLFW_KEY_NUM_LOCK:
        return Key_NumLock;
    case GLFW_KEY_PRINT_SCREEN:
        return Key_PrintScreen;
    case GLFW_KEY_PAUSE:
        return Key_Pause;
    case GLFW_KEY_F1:
        return Key_F1;
    case GLFW_KEY_F2:
        return Key_F2;
    case GLFW_KEY_F3:
        return Key_F3;
    case GLFW_KEY_F4:
        return Key_F4;
    case GLFW_KEY_F5:
        return Key_F5;
    case GLFW_KEY_F6:
        return Key_F6;
    case GLFW_KEY_F7:
        return Key_F7;
    case GLFW_KEY_F8:
        return Key_F8;
    case GLFW_KEY_F9:
        return Key_F9;
    case GLFW_KEY_F10:
        return Key_F10;
    case GLFW_KEY_F11:
        return Key_F11;
    case GLFW_KEY_F12:
        return Key_F12;
    case GLFW_KEY_F13:
        return Key_F13;
    case GLFW_KEY_F14:
        return Key_F14;
    case GLFW_KEY_F15:
        return Key_F15;
    case GLFW_KEY_F16:
        return Key_F16;
    case GLFW_KEY_F17:
        return Key_F17;
    case GLFW_KEY_F18:
        return Key_F18;
    case GLFW_KEY_F19:
        return Key_F19;
    case GLFW_KEY_F20:
        return Key_F20;
    case GLFW_KEY_F21:
        return Key_F21;
    case GLFW_KEY_F22:
        return Key_F22;
    case GLFW_KEY_F23:
        return Key_F23;
    case GLFW_KEY_F24:
        return Key_F24;
    case GLFW_KEY_F25:
        return Key_F25;
    case GLFW_KEY_KP_0:
        return Key_KP_0;
    case GLFW_KEY_KP_1:
        return Key_KP_1;
    case GLFW_KEY_KP_2:
        return Key_KP_2;
    case GLFW_KEY_KP_3:
        return Key_KP_3;
    case GLFW_KEY_KP_4:
        return Key_KP_4;
    case GLFW_KEY_KP_5:
        return Key_KP_5;
    case GLFW_KEY_KP_6:
        return Key_KP_6;
    case GLFW_KEY_KP_7:
        return Key_KP_7;
    case GLFW_KEY_KP_8:
        return Key_KP_8;
    case GLFW_KEY_KP_9:
        return Key_KP_9;
    case GLFW_KEY_KP_DECIMAL:
        return Key_KPDecimal;
    case GLFW_KEY_KP_DIVIDE:
        return Key_KPDivide;
    case GLFW_KEY_KP_MULTIPLY:
        return Key_KPMultiply;
    case GLFW_KEY_KP_SUBTRACT:
        return Key_KPSubtract;
    case GLFW_KEY_KP_ADD:
        return Key_KPAdd;
    case GLFW_KEY_KP_ENTER:
        return Key_KPEnter;
    case GLFW_KEY_KP_EQUAL:
        return Key_KPEqual;
    case GLFW_KEY_LEFT_SHIFT:
        return Key_LeftShift;
    case GLFW_KEY_LEFT_CONTROL:
        return Key_LeftControl;
    case GLFW_KEY_LEFT_ALT:
        return Key_LeftAlt;
    case GLFW_KEY_LEFT_SUPER:
        return Key_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT:
        return Key_RightShift;
    case GLFW_KEY_RIGHT_CONTROL:
        return Key_RightControl;
    case GLFW_KEY_RIGHT_ALT:
        return Key_RightAlt;
    case GLFW_KEY_RIGHT_SUPER:
        return Key_RightSuper;
    case GLFW_KEY_MENU:
        return Key_Menu;
    default:
        return Key_None;
    }
}
static Mouse toMouse(const i32 mouse)
{
    switch (mouse)
    {
    case GLFW_MOUSE_BUTTON_1:
        return Mouse_Button1;
    case GLFW_MOUSE_BUTTON_2:
        return Mouse_Button2;
    case GLFW_MOUSE_BUTTON_3:
        return Mouse_Button3;
    case GLFW_MOUSE_BUTTON_4:
        return Mouse_Button4;
    case GLFW_MOUSE_BUTTON_5:
        return Mouse_Button5;
    case GLFW_MOUSE_BUTTON_6:
        return Mouse_Button6;
    case GLFW_MOUSE_BUTTON_7:
        return Mouse_Button7;
    case GLFW_MOUSE_BUTTON_8:
        return Mouse_Button8;
        // case GLFW_MOUSE_BUTTON_LAST:
        //     return Mouse_ButtonLast;
        // case GLFW_MOUSE_BUTTON_LEFT:
        //     return Mouse_ButtonLeft;
        // case GLFW_MOUSE_BUTTON_RIGHT:
        //     return Mouse_ButtonRight;
        // case GLFW_MOUSE_BUTTON_MIDDLE:
        // return Mouse_ButtonMiddle;
    default:
        return Mouse_None;
    }
}

static InputAction toInputAction(const i32 action)
{
    switch (action)
    {
    case GLFW_PRESS:
        return InputAction_Press;
    case GLFW_RELEASE:
        return InputAction_Release;
    case GLFW_REPEAT:
        return InputAction_Repeat;
    default:
        TKIT_FATAL("[GRAPH][PLATFORM] Unknown GLFW action: {}", action);
        return InputAction_Press;
    }
}

static KeyModFlags toKeyMods(const i32 mods)
{
    KeyModFlags result = 0;
    if (mods & GLFW_MOD_SHIFT)
        result |= KeyModFlag_Shift;
    if (mods & GLFW_MOD_CONTROL)
        result |= KeyModFlag_Control;
    if (mods & GLFW_MOD_ALT)
        result |= KeyModFlag_Alt;
    if (mods & GLFW_MOD_SUPER)
        result |= KeyModFlag_Super;
    return result;
}

#define FORWARD_CALLBACK(name, ...)                                                                                    \
    const Glfw_Window *data = scast<const Glfw_Window *>(glfwGetWindowUserPointer(w));                                 \
    if (data->name)                                                                                                    \
    data->name(__VA_ARGS__)

static void glfwWindowPosCallback(GLFWwindow *w, const i32 x, const i32 y)
{
    FORWARD_CALLBACK(PositionCallback, data->Handle, x, y);
}

static void glfwWindowSizeCallback(GLFWwindow *w, const i32 width, const i32 height)
{
    FORWARD_CALLBACK(SizeCallback, data->Handle, u32(width), u32(height));
}

static void glfwFramebufferSizeCallback(GLFWwindow *w, const i32 width, const i32 height)
{
    FORWARD_CALLBACK(FramebufferSizeCallback, data->Handle, u32(width), u32(height));
}

static void glfwWindowFocusCallback(GLFWwindow *w, const i32 focused)
{
    FORWARD_CALLBACK(FocusCallback, data->Handle, focused == GLFW_TRUE);
}

static void glfwWindowCloseCallback(GLFWwindow *w)
{
    FORWARD_CALLBACK(CloseCallback, data->Handle);
}

static void glfwWindowIconifyCallback(GLFWwindow *w, const i32 iconified)
{
    FORWARD_CALLBACK(IconifyCallback, data->Handle, iconified == GLFW_TRUE);
}

static void glfwKeyCallback(GLFWwindow *w, const i32 key, const i32 scancode, const i32 action, const i32 mods)
{
    FORWARD_CALLBACK(KeyCallback, data->Handle, toKey(key), scancode, toInputAction(action), toKeyMods(mods));
}

static void glfwCharCallback(GLFWwindow *w, const u32 codepoint)
{
    FORWARD_CALLBACK(CharCallback, data->Handle, codepoint);
}

static void glfwCursorPosCallback(GLFWwindow *w, const f64 x, const f64 y)
{
    FORWARD_CALLBACK(CursorPosCallback, data->Handle, x, y);
}

static void glfwCursorEnterCallback(GLFWwindow *w, const i32 entered)
{
    FORWARD_CALLBACK(CursorEnterCallback, data->Handle, entered == GLFW_TRUE);
}

static void glfwMouseButtonCallback(GLFWwindow *w, const i32 button, const i32 action, const i32 mods)
{
    FORWARD_CALLBACK(MouseButtonCallback, data->Handle, toMouse(button), toInputAction(action), toKeyMods(mods));
}

static void glfwScrollCallback(GLFWwindow *w, const f64 xoffset, const f64 yoffset)
{
    FORWARD_CALLBACK(ScrollCallback, data->Handle, xoffset, yoffset);
}

Window Window_Create(const WindowSpecs &specs)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, specs.Flags & WindowFlag_Resizable);
    glfwWindowHint(GLFW_VISIBLE, specs.Flags & WindowFlag_Visible);
    glfwWindowHint(GLFW_DECORATED, specs.Flags & WindowFlag_Decorated);
    glfwWindowHint(GLFW_FOCUSED, specs.Flags & WindowFlag_Focused);
    glfwWindowHint(GLFW_FLOATING, specs.Flags & WindowFlag_Floating);
    // glfwWindowHint(GLFW_ICONIFIED, specs.Flags & WindowFlag_Iconified);
#ifdef GRAPH_GLFW_FOCUS_ON_SHOW
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, specs.Flags & WindowFlag_FocusOnShow);
#endif

    const Id id = s_Windows->Insert();
    Glfw_Window &win = s_Windows->At(id);

    win.Window = glfwCreateWindow(i32(specs.Dimensions[0]), i32(specs.Dimensions[1]), specs.Title, nullptr, nullptr);
    TKIT_ASSERT(win.Window, "[GRAPH][WINDOW] Failed to create window");

    win.Cursors[0] = nullptr;
    for (u32 i = 1; i < MouseCursor_Count; ++i)
        win.Cursors[i] = glfwCreateStandardCursor(toGlfw(MouseCursor(i)));

    if (specs.Position != i32v2{TKIT_I32_MAX})
    {
        TKIT_ASSERT(specs.Position[0] < TKIT_I32_MAX,
                    "[GRAPH][PLATFORM] If component y of the window position is not "
                    "TKIT_I32_MAX, component x must not be either. Passed position is ({}, {})",
                    specs.Position[0], specs.Position[1]);
        TKIT_ASSERT(specs.Position[1] < TKIT_I32_MAX,
                    "[GRAPH][PLATFORM] If component x of the window position is not "
                    "TKIT_I32_MAX, component y must not be either. Passed position is ({}, {})",
                    specs.Position[0], specs.Position[1]);

        glfwSetWindowPos(win.Window, specs.Position[0], specs.Position[1]);
    }

    win.UserData = specs.UserData;
    win.Handle = Handle_Create(Handle_Window, id);
    glfwSetWindowUserPointer(win.Window, &win);

    glfwSetWindowPosCallback(win.Window, glfwWindowPosCallback);
    glfwSetWindowSizeCallback(win.Window, glfwWindowSizeCallback);
    glfwSetFramebufferSizeCallback(win.Window, glfwFramebufferSizeCallback);
    glfwSetWindowFocusCallback(win.Window, glfwWindowFocusCallback);
    glfwSetWindowCloseCallback(win.Window, glfwWindowCloseCallback);
    glfwSetWindowIconifyCallback(win.Window, glfwWindowIconifyCallback);
    glfwSetKeyCallback(win.Window, glfwKeyCallback);
    glfwSetCharCallback(win.Window, glfwCharCallback);
    glfwSetCursorPosCallback(win.Window, glfwCursorPosCallback);
    glfwSetCursorEnterCallback(win.Window, glfwCursorEnterCallback);
    glfwSetMouseButtonCallback(win.Window, glfwMouseButtonCallback);
    glfwSetScrollCallback(win.Window, glfwScrollCallback);

    return win.Handle;
}

void Window_Destroy(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    const Id id = Handle_GetId(win);
    glfwDestroyWindow(s_Windows->At(id).Window);
    s_Windows->Remove(id);
}

void *Window_GetUserData(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return scast<const Glfw_Window *>(glfwGetWindowUserPointer(s_Windows->At(Handle_GetId(win)).Window))->UserData;
}
void Window_SetUserData(const Window win, void *data)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    scast<Glfw_Window *>(glfwGetWindowUserPointer(s_Windows->At(Handle_GetId(win)).Window))->UserData = data;
}

bool Window_ShouldClose(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwWindowShouldClose(s_Windows->At(Handle_GetId(win)).Window);
}

void Window_Show(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwShowWindow(s_Windows->At(Handle_GetId(win)).Window);
}

void Window_Hide(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwHideWindow(s_Windows->At(Handle_GetId(win)).Window);
}

void Window_Focus(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwFocusWindow(s_Windows->At(Handle_GetId(win)).Window);
}

const char *Window_GetTitle(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwGetWindowTitle(s_Windows->At(Handle_GetId(win)).Window);
}

i32v2 Window_GetPosition(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    i32 x, y;
    glfwGetWindowPos(s_Windows->At(Handle_GetId(win)).Window, &x, &y);
    return i32v2{x, y};
}

u32v2 Window_GetScreenDimensions(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    i32 w, h;
    glfwGetWindowSize(s_Windows->At(Handle_GetId(win)).Window, &w, &h);
    return u32v2{u32(w), u32(h)};
}

u32v2 Window_GetPixelDimensions(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    i32 w, h;
    glfwGetFramebufferSize(s_Windows->At(Handle_GetId(win)).Window, &w, &h);
    return u32v2{u32(w), u32(h)};
}

f32v2 Window_GetCursorPosition(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    f64 x, y;
    glfwGetCursorPos(s_Windows->At(Handle_GetId(win)).Window, &x, &y);
    return f32v2{f32(x), f32(y)};
}

f32 Window_GetAspect(const Window win)
{
    const u32v2 pdim = Window_GetPixelDimensions(win);
    return f32(pdim[0]) / f32(pdim[1]);
}

f32 Window_GetOpacity(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwGetWindowOpacity(s_Windows->At(Handle_GetId(win)).Window);
}

WindowFlags Window_GetFlags(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    GLFWwindow *handle = s_Windows->At(Handle_GetId(win)).Window;

    WindowFlags flags = 0;
    if (glfwGetWindowAttrib(handle, GLFW_RESIZABLE))
        flags |= WindowFlag_Resizable;
    if (glfwGetWindowAttrib(handle, GLFW_VISIBLE))
        flags |= WindowFlag_Visible;
    if (glfwGetWindowAttrib(handle, GLFW_DECORATED))
        flags |= WindowFlag_Decorated;
    if (glfwGetWindowAttrib(handle, GLFW_FOCUSED))
        flags |= WindowFlag_Focused;
    if (glfwGetWindowAttrib(handle, GLFW_FLOATING))
        flags |= WindowFlag_Floating;
    if (glfwGetWindowAttrib(handle, GLFW_ICONIFIED))
        flags |= WindowFlag_Iconified;
    if (glfwGetWindowAttrib(handle, GLFW_FOCUS_ON_SHOW))
        flags |= WindowFlag_FocusOnShow;
    return flags;
}

void Window_SetTitle(const Window win, const char *title)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwSetWindowTitle(s_Windows->At(Handle_GetId(win)).Window, title);
}

void Window_SetPosition(const Window win, const i32v2 &pos)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwSetWindowPos(s_Windows->At(Handle_GetId(win)).Window, pos[0], pos[1]);
}

void Window_SetScreenDimensions(const Window win, const u32v2 &dim)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    TKIT_ASSERT(dim[0] != 0 && dim[1] != 0,
                "[GRAPH][WINDOW] Cannot have window dimensions of zero! Passed values: {}, {}", dim[0], dim[1]);
    glfwSetWindowSize(s_Windows->At(Handle_GetId(win)).Window, i32(dim[0]), i32(dim[1]));
}

void Window_SetAspect(const Window win, const u32 numer, const u32 denom)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwSetWindowAspectRatio(s_Windows->At(Handle_GetId(win)).Window, i32(numer), i32(denom));
}

void Window_SetFlags(const Window win, const WindowFlags flags)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    GLFWwindow *handle = s_Windows->At(Handle_GetId(win)).Window;

    glfwSetWindowAttrib(handle, GLFW_RESIZABLE, bool(flags & WindowFlag_Resizable));
    glfwSetWindowAttrib(handle, GLFW_DECORATED, bool(flags & WindowFlag_Decorated));
    glfwSetWindowAttrib(handle, GLFW_FLOATING, bool(flags & WindowFlag_Floating));
    glfwSetWindowAttrib(handle, GLFW_FOCUS_ON_SHOW, bool(flags & WindowFlag_FocusOnShow));
}

void Window_AddFlags(const Window win, const WindowFlags flags)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    GLFWwindow *handle = s_Windows->At(Handle_GetId(win)).Window;

    if (flags & WindowFlag_Resizable)
        glfwSetWindowAttrib(handle, GLFW_RESIZABLE, GLFW_TRUE);
    if (flags & WindowFlag_Decorated)
        glfwSetWindowAttrib(handle, GLFW_DECORATED, GLFW_TRUE);
    if (flags & WindowFlag_Floating)
        glfwSetWindowAttrib(handle, GLFW_FLOATING, GLFW_TRUE);
    if (flags & WindowFlag_FocusOnShow)
        glfwSetWindowAttrib(handle, GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
}

void Window_RemoveFlags(const Window win, const WindowFlags flags)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    GLFWwindow *handle = s_Windows->At(Handle_GetId(win)).Window;

    if (flags & WindowFlag_Resizable)
        glfwSetWindowAttrib(handle, GLFW_RESIZABLE, GLFW_FALSE);
    if (flags & WindowFlag_Decorated)
        glfwSetWindowAttrib(handle, GLFW_DECORATED, GLFW_FALSE);
    if (flags & WindowFlag_Floating)
        glfwSetWindowAttrib(handle, GLFW_FLOATING, GLFW_FALSE);
    if (flags & WindowFlag_FocusOnShow)
        glfwSetWindowAttrib(handle, GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
}

void Window_SetOpacity(const Window win, const f32 opacity)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwSetWindowOpacity(s_Windows->At(Handle_GetId(win)).Window, opacity);
}

void Window_SetMouseCursor(const Window win, const MouseCursor cursor)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    glfwSetCursor(data.Window, data.Cursors[cursor]);
}

void Window_SignalShouldClose(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    glfwSetWindowShouldClose(s_Windows->At(Handle_GetId(win)).Window, GLFW_TRUE);
}

bool Window_IsKeyPressed(const Window win, const Key key)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwGetKey(s_Windows->At(Handle_GetId(win)).Window, toGlfw(key)) == GLFW_PRESS;
}

bool Window_IsKeyReleased(const Window win, const Key key)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwGetKey(s_Windows->At(Handle_GetId(win)).Window, toGlfw(key)) == GLFW_RELEASE;
}

bool Window_IsMousePressed(const Window win, const Mouse button)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwGetMouseButton(s_Windows->At(Handle_GetId(win)).Window, toGlfw(button)) == GLFW_PRESS;
}

bool Window_IsMouseReleased(const Window win, const Mouse button)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    return glfwGetMouseButton(s_Windows->At(Handle_GetId(win)).Window, toGlfw(button)) == GLFW_RELEASE;
}

Monitor Window_GetMonitor(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Monitor);
    GLFWwindow *w = GetWindow(win);
    GLFWmonitor *mon = glfwGetWindowMonitor(w);
    if (!mon)
        return NullHandle;

    return scast<const Glfw_Monitor *>(glfwGetMonitorUserPointer(mon))->Handle;
}

bool Window_IsHandleValid(const Window win)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Windows, win, Handle_Window);
}

Monitor Monitor_GetPrimary()
{
    return s_Monitors->IsEmpty() ? NullHandle : Handle_Create(Handle_Monitor, 0);
}

void *Monitor_GetUserData(const Monitor monitor)
{
    GRAPH_CHECK_HANDLE(monitor, Handle_Monitor);
    return scast<const Glfw_Monitor *>(glfwGetMonitorUserPointer(s_Monitors->At(Handle_GetId(monitor)).Monitor))
        ->UserData;
}
void Monitor_SetUserData(const Monitor monitor, void *data)
{
    GRAPH_CHECK_HANDLE(monitor, Handle_Monitor);
    scast<Glfw_Monitor *>(glfwGetMonitorUserPointer(s_Monitors->At(Handle_GetId(monitor)).Monitor))->UserData = data;
}

i32v2 Monitor_GetPosition(const Monitor monitor)
{
    GRAPH_CHECK_HANDLE(monitor, Handle_Monitor);
    GLFWmonitor *mon = s_Monitors->At(Handle_GetId(monitor)).Monitor;
    i32 x, y;
    glfwGetMonitorPos(mon, &x, &y);
    return {x, y};
}

VideoMode Monitor_GetVideoMode(const Monitor monitor)
{
    GRAPH_CHECK_HANDLE(monitor, Handle_Monitor);
    GLFWmonitor *mon = s_Monitors->At(Handle_GetId(monitor)).Monitor;
    const GLFWvidmode *mode = glfwGetVideoMode(mon);
    TKIT_ASSERT(mode, "[GRAPH][PLATFORM] Failed to get video mode");

    VideoMode vmode;
    vmode.Dimensions = {u32(mode->width), u32(mode->height)};
    vmode.BitDepths = {u32(mode->redBits), u32(mode->greenBits), u32(mode->blueBits)};
    vmode.RefreshRate = u32(mode->refreshRate);

    return vmode;
}

bool Monitor_IsHandleValid(const Monitor monitor)
{
    if (Handle_GetType(monitor) != Handle_Monitor)
        return false;

    return Handle_GetId(monitor) < s_Monitors->GetSize();
}

WindowPosCallback Window_Callback_Position(const Window win, const WindowPosCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    WindowPosCallback prev = data.PositionCallback;
    data.PositionCallback = callback;
    glfwSetWindowPosCallback(data.Window, callback ? glfwWindowPosCallback : nullptr);
    return prev;
}

WindowSizeCallback Window_Callback_Size(const Window win, const WindowSizeCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    WindowSizeCallback prev = data.SizeCallback;
    data.SizeCallback = callback;
    glfwSetWindowSizeCallback(data.Window, callback ? glfwWindowSizeCallback : nullptr);
    return prev;
}

FramebufferSizeCallback Window_Callback_FramebufferSize(const Window win, const FramebufferSizeCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    FramebufferSizeCallback prev = data.FramebufferSizeCallback;
    data.FramebufferSizeCallback = callback;
    glfwSetFramebufferSizeCallback(data.Window, callback ? glfwFramebufferSizeCallback : nullptr);
    return prev;
}

WindowFocusCallback Window_Callback_Focus(const Window win, const WindowFocusCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    WindowFocusCallback prev = data.FocusCallback;
    data.FocusCallback = callback;
    glfwSetWindowFocusCallback(data.Window, callback ? glfwWindowFocusCallback : nullptr);
    return prev;
}

WindowCloseCallback Window_Callback_Close(const Window win, const WindowCloseCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    WindowCloseCallback prev = data.CloseCallback;
    data.CloseCallback = callback;
    glfwSetWindowCloseCallback(data.Window, callback ? glfwWindowCloseCallback : nullptr);
    return prev;
}

WindowIconifyCallback Window_Callback_Iconify(const Window win, const WindowIconifyCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    WindowIconifyCallback prev = data.IconifyCallback;
    data.IconifyCallback = callback;
    glfwSetWindowIconifyCallback(data.Window, callback ? glfwWindowIconifyCallback : nullptr);
    return prev;
}

KeyCallback Window_Callback_Key(const Window win, const KeyCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    KeyCallback prev = data.KeyCallback;
    data.KeyCallback = callback;
    glfwSetKeyCallback(data.Window, callback ? glfwKeyCallback : nullptr);
    return prev;
}

CharCallback Window_Callback_Char(const Window win, const CharCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    CharCallback prev = data.CharCallback;
    data.CharCallback = callback;
    glfwSetCharCallback(data.Window, callback ? glfwCharCallback : nullptr);
    return prev;
}

CursorPosCallback Window_Callback_CursorPos(const Window win, const CursorPosCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    CursorPosCallback prev = data.CursorPosCallback;
    data.CursorPosCallback = callback;
    glfwSetCursorPosCallback(data.Window, callback ? glfwCursorPosCallback : nullptr);
    return prev;
}

CursorEnterCallback Window_Callback_CursorEnter(const Window win, const CursorEnterCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    CursorEnterCallback prev = data.CursorEnterCallback;
    data.CursorEnterCallback = callback;
    glfwSetCursorEnterCallback(data.Window, callback ? glfwCursorEnterCallback : nullptr);
    return prev;
}

MouseButtonCallback Window_Callback_MouseButton(const Window win, const MouseButtonCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    MouseButtonCallback prev = data.MouseButtonCallback;
    data.MouseButtonCallback = callback;
    glfwSetMouseButtonCallback(data.Window, callback ? glfwMouseButtonCallback : nullptr);
    return prev;
}

ScrollCallback Window_Callback_Scroll(const Window win, const ScrollCallback callback)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);
    auto &data = s_Windows->At(Handle_GetId(win));
    ScrollCallback prev = data.ScrollCallback;
    data.ScrollCallback = callback;
    glfwSetScrollCallback(data.Window, callback ? glfwScrollCallback : nullptr);
    return prev;
}

void Platform_Initialize(const Platform plat, const u32 maxWindows)
{
    s_Windows.Construct();
    s_Monitors.Construct();
    s_Windows->Reserve(maxWindows);

#ifdef TKIT_ENABLE_ERROR_LOGS
    glfwSetErrorCallback(glfwErrorCallback);
#endif
    glfwInitHint(GLFW_PLATFORM, toGlfw(plat));
    TKIT_ENSURE_RETURNS(glfwInit(), GLFW_TRUE, "[GRAPH][PLATFORM] GLFW failed to initialize");

    TKIT_LOG_WARNING_IF(!glfwVulkanSupported(), "[GRAPH][PLATFORM] Vulkan is not supported, according to GLFW");

    i32 mcount;
    GLFWmonitor **monitors = glfwGetMonitors(&mcount);
    if (monitors)
    {
        s_Monitors->Reserve(mcount);
        for (i32 i = 0; i < mcount; ++i)
            glfwSetMonitorUserPointer(monitors[i],
                                      &s_Monitors->Append(monitors[i], nullptr, Handle_Create(Handle_Monitor, i)));
    }
}
void Platform_Terminate()
{
    glfwTerminate();
    s_Windows.Destruct();
}

GLFWwindow *GetWindow(const Window win)
{
    GRAPH_CHECK_HANDLE(win, Handle_Window);

    return s_Windows->At(Handle_GetId(win)).Window;
}
} // namespace Graph
