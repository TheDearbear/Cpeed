#include <dcimgui.h>
#include <malloc.h>

#include "../../platform/input/mode.h"
#include "../../platform/frame.h"
#include "../../platform.h"
#include "imgui_impl_cpeed.h"

#define ImGuiMouseButton_Extra1 3
#define ImGuiMouseButton_Extra2 4

typedef struct ImGui_ImplCpeed_Data {
    CpdWindow window;
    uint64_t time;
    uint32_t frame_layer_handle;

    uint32_t wanted_text_input : 1;
} ImGui_ImplCpeed_Data;

static ImGui_ImplCpeed_Data* cImGui_ImplCpeed_GetBackendData()
{
    return ImGui_GetCurrentContext() ? (ImGui_ImplCpeed_Data*)ImGui_GetIO()->BackendPlatformUserData : 0;
}

static ImGuiKey map_key(CpdKeyCode key_code) {
    if (key_code >= CpdKeyCode_A && key_code <= CpdKeyCode_Z) {
        return key_code + (ImGuiKey_A - CpdKeyCode_A);
    }

    if (key_code >= CpdKeyCode_0 && key_code <= CpdKeyCode_9) {
        return key_code + (ImGuiKey_0 - CpdKeyCode_0);
    }

    if (key_code >= CpdKeyCode_Numpad0 && key_code <= CpdKeyCode_Numpad9) {
        return key_code + (ImGuiKey_Keypad0 - CpdKeyCode_Numpad0);
    }

    if (key_code >= CpdKeyCode_F1 && key_code <= CpdKeyCode_F24) {
        return key_code + (ImGuiKey_F1 - CpdKeyCode_F1);
    }

    switch (key_code) {
        case CpdKeyCode_Tab:          return ImGuiKey_Tab;
        case CpdKeyCode_LeftArrow:    return ImGuiKey_LeftArrow;
        case CpdKeyCode_RightArrow:   return ImGuiKey_RightArrow;
        case CpdKeyCode_UpArrow:      return ImGuiKey_UpArrow;
        case CpdKeyCode_DownArrow:    return ImGuiKey_DownArrow;
        case CpdKeyCode_PageUp:       return ImGuiKey_PageUp;
        case CpdKeyCode_PageDown:     return ImGuiKey_PageDown;
        case CpdKeyCode_Home:         return ImGuiKey_Home;
        case CpdKeyCode_End:          return ImGuiKey_End;
        case CpdKeyCode_Insert:       return ImGuiKey_Insert;
        case CpdKeyCode_Delete:       return ImGuiKey_Delete;
        case CpdKeyCode_Backspace:    return ImGuiKey_Backspace;
        case CpdKeyCode_Spacebar:     return ImGuiKey_Space;
        case CpdKeyCode_Enter:        return ImGuiKey_Enter;
        case CpdKeyCode_NumpadEnter:  return ImGuiKey_KeypadEnter;
        case CpdKeyCode_Escape:       return ImGuiKey_Escape;
        case CpdKeyCode_LeftControl:  return ImGuiKey_LeftCtrl;
        case CpdKeyCode_LeftShift:    return ImGuiKey_LeftShift;
        case CpdKeyCode_LeftAlt:      return ImGuiKey_LeftAlt;
        case CpdKeyCode_RightControl: return ImGuiKey_RightCtrl;
        case CpdKeyCode_RightShift:   return ImGuiKey_RightShift;
        case CpdKeyCode_RightAlt:     return ImGuiKey_RightAlt;
        case CpdKeyCode_Quote:        return ImGuiKey_Apostrophe;
        case CpdKeyCode_Comma:        return ImGuiKey_Comma;
        case CpdKeyCode_Minus:        return ImGuiKey_Minus;
        case CpdKeyCode_Dot:          return ImGuiKey_Period;
        case CpdKeyCode_Slash:        return ImGuiKey_Slash;
        case CpdKeyCode_Semicolon:    return ImGuiKey_Semicolon;
        case CpdKeyCode_Equal:        return ImGuiKey_Equal;
        case CpdKeyCode_LeftSquareBracket:  return ImGuiKey_LeftBracket;
        case CpdKeyCode_Backslash:    return ImGuiKey_Backslash;
        case CpdKeyCode_RightSquareBracket: return ImGuiKey_RightBracket;
        case CpdKeyCode_Backtick:     return ImGuiKey_GraveAccent;
        case CpdKeyCode_Multiply:     return ImGuiKey_KeypadMultiply;
        case CpdKeyCode_Plus:         return ImGuiKey_KeypadAdd;
    }

    return ImGuiKey_None;
}

static bool input(CpdWindow window, CpdFrame* frame, const CpdInputEvent* event) {
    ImGui_ImplCpeed_Data* data = cImGui_ImplCpeed_GetBackendData();
    ImGuiIO* io = ImGui_GetIO();

    if (data->wanted_text_input != io->WantTextInput) {
        data->wanted_text_input = io->WantTextInput;

        set_window_input_mode(window, data->wanted_text_input ? CpdInputMode_Text : CpdInputMode_KeyCode);
    }

    if (event->type == CpdInputEventType_MouseMove) {
        ImGuiIO_AddMousePosEvent(io, (float)event->data.mouse_move.x_pos, (float)event->data.mouse_move.y_pos);

        return !io->WantCaptureMouse;
    }
    else if (event->type == CpdInputEventType_MouseButtonPress) {
        ImGuiMouseButton button;
        switch (event->data.mouse_button_press.button) {
            case CpdMouseButtonType_Left:
                button = ImGuiMouseButton_Left;
                break;

            case CpdMouseButtonType_Middle:
                button = ImGuiMouseButton_Middle;
                break;

            case CpdMouseButtonType_Right:
                button = ImGuiMouseButton_Right;
                break;

            case CpdMouseButtonType_Extra1:
                button = ImGuiMouseButton_Extra1;
                break;

            case CpdMouseButtonType_Extra2:
                button = ImGuiMouseButton_Extra2;
                break;

            default:
                return true;
        }

        ImGuiIO_AddMouseButtonEvent(io, button, event->data.mouse_button_press.pressed);

        return !io->WantCaptureMouse;
    }
    else if (event->type == CpdInputEventType_MouseScroll) {
        ImGuiIO_AddMouseWheelEvent(io, (float)event->data.mouse_scroll.horizontal_scroll / 120, (float)-event->data.mouse_scroll.vertical_scroll / 120);

        return !io->WantCaptureMouse;
    }
    else if (event->type == CpdInputEventType_CharInput && io->WantTextInput) {
        ImGuiIO_AddInputCharacter(io, event->data.char_input.character);

        return false;
    }
    else if (event->type == CpdInputEventType_TextInput && io->WantTextInput) {
        ImGuiIO_AddInputCharactersUTF8(io, event->data.text_input.text);

        return false;
    }
    else if (event->type == CpdInputEventType_ButtonPress) {
        ImGuiKey key = map_key(event->data.button_press.key_code);

        // Linux is using custom handling due to technical reasons
        if (compile_platform() != CpdCompilePlatform_Linux) {
            if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl) {
                ImGuiIO_AddKeyEvent(io, ImGuiMod_Ctrl, event->data.button_press.pressed || (event->modifiers & CpdInputModifierKey_Control) != 0);
            }
            else if (key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift) {
                ImGuiIO_AddKeyEvent(io, ImGuiMod_Shift, event->data.button_press.pressed || (event->modifiers & CpdInputModifierKey_Shift) != 0);
            }
            else if (key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt) {
                ImGuiIO_AddKeyEvent(io, ImGuiMod_Alt, event->data.button_press.pressed || (event->modifiers & CpdInputModifierKey_Alt) != 0);
            }
        }

        ImGuiIO_AddKeyEvent(io, key, event->data.button_press.pressed);

        return !io->WantCaptureKeyboard;
    }
    
    return true;
}

static void resize(CpdWindow window, CpdFrame* frame, CpdSize size) {
    ImGuiIO* io = ImGui_GetIO();

    io->DisplaySize = (ImVec2) {
        .x = (float)size.width,
        .y = (float)size.height
    };
}

CIMGUI_IMPL_API bool cImGui_ImplCpeed_Init(CpdWindow window) {
    ImGui_ImplCpeed_Data* data = (ImGui_ImplCpeed_Data*)malloc(sizeof(ImGui_ImplCpeed_Data));
    if (data == 0) {
        return false;
    }

    data->window = window;
    data->time = get_clock_usec();

    data->wanted_text_input = false;

    ImGuiIO* io = ImGui_GetIO();
    io->BackendPlatformUserData = (void*)data;
    io->BackendPlatformName = "imgui_impl_cpeed";

    CpdSize size = window_size(window);
    io->DisplaySize = (ImVec2) {
        .x = (float)size.width,
        .y = (float)size.height
    };

    ImGuiViewport* main_viewport = ImGui_GetMainViewport();
    main_viewport->PlatformHandle = window;
    // PlatformHandleRaw is used for MultiViewport support
    main_viewport->PlatformHandleRaw = 0;

    CpdFrameLayerFunctions functions = {
        .imgui = 0,
        .input = input,
        .render = 0,
        .resize = resize
    };

    add_frame_layer(window, &functions, CpdFrameLayerFlags_None);

    return true;
}

CIMGUI_IMPL_API void cImGui_ImplCpeed_Shutdown(void) {
    ImGui_ImplCpeed_Data* data = cImGui_ImplCpeed_GetBackendData();
    IM_ASSERT(data != 0 && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO* io = ImGui_GetIO();

    io->BackendPlatformName = 0;
    io->BackendPlatformUserData = 0;

    remove_frame_layer(data->window, data->frame_layer_handle);

    free(data);
}

CIMGUI_IMPL_API void cImGui_ImplCpeed_NewFrame(void) {
    ImGui_ImplCpeed_Data* data = cImGui_ImplCpeed_GetBackendData();
    ImGuiIO* io = ImGui_GetIO();

    uint64_t current_time = get_clock_usec();
    io->DeltaTime = (float)(current_time - data->time) / 1000000;
    data->time = current_time;
}
