#include "pch.hpp"
#include "graph/core.hpp"
#ifdef GRAPH_HAS_PLATFORM_BACKEND
#    include "graph/platform.hpp"
#    include "graph/swap_chain.hpp"
#endif
#include "graph/execution.hpp"
#include "graph/resources.hpp"
#include "graph/shader.hpp"

namespace Graph
{
bool Handle_IsValid(const Handle handle, const HandleType htype)
{
    const HandleType itype = Handle_GetType(handle);
    if (itype >= Handle_Count || (itype != htype && htype != Handle_None))
        return false;

    switch (itype)
    {
#ifdef GRAPH_HAS_PLATFORM_BACKEND
    case Handle_Window:
        return Window_IsHandleValid(handle);
    case Handle_Monitor:
        return Monitor_IsHandleValid(handle);
    case Handle_Swapchain:
        return Swapchain_IsHandleValid(handle);
#endif
    case Handle_CommandPool:
        return CommandPool_IsHandleValid(handle);
    case Handle_CommandBuffer:
        return CommandBuffer_IsHandleValid(handle);
    case Handle_Buffer:
        return Buffer_IsHandleValid(handle);
    case Handle_Image:
        return Image_IsHandleValid(handle);
    case Handle_ImageView:
        return ImageView_IsHandleValid(handle);
    case Handle_Sampler:
        return Sampler_IsHandleValid(handle);
    case Handle_DescriptorSet:
        return DescriptorSet_IsHandleValid(handle);
    case Handle_DescriptorSetLayout:
        return DescriptorSetLayout_IsHandleValid(handle);
    case Handle_Shader:
        return Shader_IsHandleValid(handle);
#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
    case Handle_Compilation:
        return Compilation_IsHandleValid(handle);
#endif
#ifdef GRAPH_HAS_SHADER_REFLECTION_BACKEND
    case Handle_Reflection:
        return Reflection_IsHandleValid(handle);
#endif
    case Handle_PipelineLayout:
        return PipelineLayout_IsHandleValid(handle);
    case Handle_Pipeline:
        return Pipeline_IsHandleValid(handle);
    default:
        return false;
    }
}

const char *ToString(const HandleType htype)
{
    switch (htype)
    {
    case Handle_Window:
        return "Handle_Window";
    case Handle_Monitor:
        return "Handle_Monitor";
    case Handle_Swapchain:
        return "Handle_Swapchain";
    case Handle_CommandPool:
        return "Handle_CommandPool";
    case Handle_CommandBuffer:
        return "Handle_CommandBuffer";
    case Handle_Buffer:
        return "Handle_Buffer";
    case Handle_Image:
        return "Handle_Image";
    case Handle_ImageView:
        return "Handle_ImageView";
    case Handle_Sampler:
        return "Handle_Sampler";
    case Handle_DescriptorSetLayout:
        return "Handle_DescriptorSetLayout";
    case Handle_DescriptorSet:
        return "Handle_DescriptorSet";
    case Handle_Shader:
        return "Handle_Shader";
    case Handle_Compilation:
        return "Handle_Compilation";
    case Handle_Reflection:
        return "Handle_Reflection";
    case Handle_PipelineLayout:
        return "Handle_PipelineLayout";
    case Handle_Pipeline:
        return "Handle_Pipeline";
    case Handle_None:
        return "Handle_None";
    default:
        TKIT_ASSERT(false, "[GRAPH] Unknown handle type: {}", u32(htype));
        return "Handle_Unknown";
    }
}
#define CASE_TO_STRING(name)                                                                                           \
    case name:                                                                                                         \
        return #name

const char *ToString(const Key key)
{
    switch (key)
    {
        CASE_TO_STRING(Key_Space);
        CASE_TO_STRING(Key_Apostrophe);
        CASE_TO_STRING(Key_Comma);
        CASE_TO_STRING(Key_Minus);
        CASE_TO_STRING(Key_Period);
        CASE_TO_STRING(Key_Slash);
        CASE_TO_STRING(Key_N0);
        CASE_TO_STRING(Key_N1);
        CASE_TO_STRING(Key_N2);
        CASE_TO_STRING(Key_N3);
        CASE_TO_STRING(Key_N4);
        CASE_TO_STRING(Key_N5);
        CASE_TO_STRING(Key_N6);
        CASE_TO_STRING(Key_N7);
        CASE_TO_STRING(Key_N8);
        CASE_TO_STRING(Key_N9);
        CASE_TO_STRING(Key_Semicolon);
        CASE_TO_STRING(Key_Equal);
        CASE_TO_STRING(Key_A);
        CASE_TO_STRING(Key_B);
        CASE_TO_STRING(Key_C);
        CASE_TO_STRING(Key_D);
        CASE_TO_STRING(Key_E);
        CASE_TO_STRING(Key_F);
        CASE_TO_STRING(Key_G);
        CASE_TO_STRING(Key_H);
        CASE_TO_STRING(Key_I);
        CASE_TO_STRING(Key_J);
        CASE_TO_STRING(Key_K);
        CASE_TO_STRING(Key_L);
        CASE_TO_STRING(Key_M);
        CASE_TO_STRING(Key_N);
        CASE_TO_STRING(Key_O);
        CASE_TO_STRING(Key_P);
        CASE_TO_STRING(Key_Q);
        CASE_TO_STRING(Key_R);
        CASE_TO_STRING(Key_S);
        CASE_TO_STRING(Key_T);
        CASE_TO_STRING(Key_U);
        CASE_TO_STRING(Key_V);
        CASE_TO_STRING(Key_W);
        CASE_TO_STRING(Key_X);
        CASE_TO_STRING(Key_Y);
        CASE_TO_STRING(Key_Z);
        CASE_TO_STRING(Key_LeftBracket);
        CASE_TO_STRING(Key_Backslash);
        CASE_TO_STRING(Key_RightBracket);
        CASE_TO_STRING(Key_GraveAccent);
        CASE_TO_STRING(Key_World_1);
        CASE_TO_STRING(Key_World_2);
        CASE_TO_STRING(Key_Escape);
        CASE_TO_STRING(Key_Enter);
        CASE_TO_STRING(Key_Tab);
        CASE_TO_STRING(Key_Backspace);
        CASE_TO_STRING(Key_Insert);
        CASE_TO_STRING(Key_Delete);
        CASE_TO_STRING(Key_Right);
        CASE_TO_STRING(Key_Left);
        CASE_TO_STRING(Key_Down);
        CASE_TO_STRING(Key_Up);
        CASE_TO_STRING(Key_PageUp);
        CASE_TO_STRING(Key_PageDown);
        CASE_TO_STRING(Key_Home);
        CASE_TO_STRING(Key_End);
        CASE_TO_STRING(Key_CapsLock);
        CASE_TO_STRING(Key_ScrollLock);
        CASE_TO_STRING(Key_NumLock);
        CASE_TO_STRING(Key_PrintScreen);
        CASE_TO_STRING(Key_Pause);
        CASE_TO_STRING(Key_F1);
        CASE_TO_STRING(Key_F2);
        CASE_TO_STRING(Key_F3);
        CASE_TO_STRING(Key_F4);
        CASE_TO_STRING(Key_F5);
        CASE_TO_STRING(Key_F6);
        CASE_TO_STRING(Key_F7);
        CASE_TO_STRING(Key_F8);
        CASE_TO_STRING(Key_F9);
        CASE_TO_STRING(Key_F10);
        CASE_TO_STRING(Key_F11);
        CASE_TO_STRING(Key_F12);
        CASE_TO_STRING(Key_F13);
        CASE_TO_STRING(Key_F14);
        CASE_TO_STRING(Key_F15);
        CASE_TO_STRING(Key_F16);
        CASE_TO_STRING(Key_F17);
        CASE_TO_STRING(Key_F18);
        CASE_TO_STRING(Key_F19);
        CASE_TO_STRING(Key_F20);
        CASE_TO_STRING(Key_F21);
        CASE_TO_STRING(Key_F22);
        CASE_TO_STRING(Key_F23);
        CASE_TO_STRING(Key_F24);
        CASE_TO_STRING(Key_F25);
        CASE_TO_STRING(Key_KP_0);
        CASE_TO_STRING(Key_KP_1);
        CASE_TO_STRING(Key_KP_2);
        CASE_TO_STRING(Key_KP_3);
        CASE_TO_STRING(Key_KP_4);
        CASE_TO_STRING(Key_KP_5);
        CASE_TO_STRING(Key_KP_6);
        CASE_TO_STRING(Key_KP_7);
        CASE_TO_STRING(Key_KP_8);
        CASE_TO_STRING(Key_KP_9);
        CASE_TO_STRING(Key_KPDecimal);
        CASE_TO_STRING(Key_KPDivide);
        CASE_TO_STRING(Key_KPMultiply);
        CASE_TO_STRING(Key_KPSubtract);
        CASE_TO_STRING(Key_KPAdd);
        CASE_TO_STRING(Key_KPEnter);
        CASE_TO_STRING(Key_KPEqual);
        CASE_TO_STRING(Key_LeftShift);
        CASE_TO_STRING(Key_LeftControl);
        CASE_TO_STRING(Key_LeftAlt);
        CASE_TO_STRING(Key_LeftSuper);
        CASE_TO_STRING(Key_RightShift);
        CASE_TO_STRING(Key_RightControl);
        CASE_TO_STRING(Key_RightAlt);
        CASE_TO_STRING(Key_RightSuper);
        CASE_TO_STRING(Key_Menu);
        CASE_TO_STRING(Key_None);
    default:
        return "Unknown";
    }
}

const char *ToString(const Mouse button)
{
    switch (button)
    {
        CASE_TO_STRING(Mouse_Button1);
        CASE_TO_STRING(Mouse_Button2);
        CASE_TO_STRING(Mouse_Button3);
        CASE_TO_STRING(Mouse_Button4);
        CASE_TO_STRING(Mouse_Button5);
        CASE_TO_STRING(Mouse_Button6);
        CASE_TO_STRING(Mouse_Button7);
        CASE_TO_STRING(Mouse_Button8);
        CASE_TO_STRING(Mouse_None);
    default:
        return "Unknown";
    }
}

#undef CASE_TO_STRING
} // namespace Graph
