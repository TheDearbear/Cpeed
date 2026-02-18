#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <windowsx.h>

#include <Cpeed/platform/logging.h>
#include <Cpeed/platform/window.h>
#include <Cpeed/platform.h>

#include "winmain.h"
#include "winproc.h"

static LRESULT add_button_press_to_queue(WindowExtraData* data, bool pressed, CpdKeyCode keyCode) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_ButtonPress,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.button_press.key_code = keyCode,
        .data.button_press.pressed = pressed
    };

    return 0;
}

static LRESULT add_mouse_scroll_to_queue(WindowExtraData* data, int32_t vertical, int32_t horizontal) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseScroll,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_scroll.vertical_scroll = vertical,
        .data.mouse_scroll.horizontal_scroll = horizontal
    };

    return 0;
}

static LRESULT add_char_input_to_queue(WindowExtraData* data, uint32_t character, uint32_t length) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_CharInput,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.char_input.character = character,
        .data.char_input.length = length
    };

    return 0;
}

static LRESULT add_mouse_button_press_to_queue(WindowExtraData* data, CpdMouseButtonType button, bool pressed) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseButtonPress,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_button_press.button = button,
        .data.mouse_button_press.pressed = pressed
    };

    return 0;
}

static LRESULT add_mouse_move_to_queue(WindowExtraData* data, int32_t new_x, int32_t new_y) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    int32_t delta_x = 0;
    int32_t delta_y = 0;

    if (data->first_mouse_event) {
        data->first_mouse_event = false;
    }
    else {
        delta_x = new_x - data->mouse_x;
        delta_y = new_y - data->mouse_y;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseMove,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_move.x_pos = (int16_t)new_x,
        .data.mouse_move.y_pos = (int16_t)new_y,
        .data.mouse_move.x_move = (int16_t)delta_x,
        .data.mouse_move.y_move = (int16_t)delta_y
    };

    return 0;
}

static void set_key_modifier(WindowExtraData* data, CpdInputModifierKeyFlags modifier, bool enable) {
    if (enable) {
        data->current_key_modifiers |= modifier;
    }
    else {
        data->current_key_modifiers &= ~modifier;
    }
}

static CpdKeyCode virtual_key_to_key_code(USHORT virtual_key, USHORT scan_code) {
    if (virtual_key >= VK_F1 && virtual_key <= VK_F24) {
        return virtual_key - (VK_F1 - CpdKeyCode_F1);
    }

    if (virtual_key >= 'A' && virtual_key <= 'Z') {
        return virtual_key - ('A' - CpdKeyCode_A);
    }

    if (virtual_key >= '0' && virtual_key <= '9') {
        return virtual_key - ('0' - CpdKeyCode_0);
    }

    if (virtual_key >= VK_NUMPAD0 && virtual_key <= VK_NUMPAD9) {
        return virtual_key - (VK_NUMPAD0 - CpdKeyCode_Numpad0);
    }

    if (virtual_key == VK_SHIFT || virtual_key == VK_CONTROL || virtual_key == VK_MENU) {
        virtual_key = LOWORD(MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX));
    }

    if (virtual_key == VK_RETURN && (scan_code & 0xE000) != 0) {
        return CpdKeyCode_NumpadEnter;
    }

    switch (virtual_key) {
        case VK_BACK:      return CpdKeyCode_Backspace;
        case VK_TAB:       return CpdKeyCode_Tab;
        case VK_RETURN:    return CpdKeyCode_Enter;
        case VK_LSHIFT:    return CpdKeyCode_LeftShift;
        case VK_RSHIFT:    return CpdKeyCode_RightShift;
        case VK_LCONTROL:  return CpdKeyCode_LeftControl;
        case VK_RCONTROL:  return CpdKeyCode_RightControl;
        case VK_LMENU:     return CpdKeyCode_LeftAlt;
        case VK_RMENU:     return CpdKeyCode_RightAlt;
        case VK_ESCAPE:    return CpdKeyCode_Escape;
        case VK_SPACE:     return CpdKeyCode_Spacebar;
        case VK_PRIOR:     return CpdKeyCode_PageUp;
        case VK_NEXT:      return CpdKeyCode_PageDown;
        case VK_END:       return CpdKeyCode_End;
        case VK_HOME:      return CpdKeyCode_Home;
        case VK_LEFT:      return CpdKeyCode_LeftArrow;
        case VK_RIGHT:     return CpdKeyCode_RightArrow;
        case VK_UP:        return CpdKeyCode_UpArrow;
        case VK_DOWN:      return CpdKeyCode_DownArrow;
        case VK_INSERT:    return CpdKeyCode_Insert;
        case VK_DELETE:    return CpdKeyCode_Delete;
        case VK_MULTIPLY:  return CpdKeyCode_Multiply;
        case VK_SUBTRACT:  case VK_OEM_MINUS:  return CpdKeyCode_Minus;
        case VK_ADD:       return CpdKeyCode_Plus;
        case VK_OEM_PLUS:  return CpdKeyCode_Equal;
        case VK_DIVIDE:    case VK_OEM_2:      return CpdKeyCode_Slash;
        case VK_OEM_5:     case VK_OEM_102:    return CpdKeyCode_Backslash;
        case VK_DECIMAL:   case VK_OEM_PERIOD: return CpdKeyCode_Dot;
        case VK_OEM_COMMA: return CpdKeyCode_Comma;
        case VK_OEM_1:     return CpdKeyCode_Semicolon;
        case VK_OEM_4:     return CpdKeyCode_LeftSquareBracket;
        case VK_OEM_6:     return CpdKeyCode_RightSquareBracket;
        case VK_OEM_7:     return CpdKeyCode_Quote;
        case VK_OEM_3:     return CpdKeyCode_Backtick;
        default:           return CpdKeyCode_Invalid;
    }
}

static bool check_key_code(WindowExtraData* data, CpdKeyCode keyCode, bool pressed) {
    if (keyCode == CpdKeyCode_LeftShift) {
        if (data->left_shift_pressed != pressed) {
            data->left_shift_pressed = pressed;
        }
        else {
            return false;
        }
    }
    else if (keyCode == CpdKeyCode_RightShift) {
        if (data->right_shift_pressed != pressed) {
            data->right_shift_pressed = pressed;
        }
        else {
            return false;
        }
    }
    else if (keyCode == CpdKeyCode_LeftControl) {
        if (data->left_control_pressed != pressed) {
            data->left_control_pressed = pressed;
        }
        else {
            return false;
        }
    }
    else if (keyCode == CpdKeyCode_RightControl) {
        if (data->right_control_pressed != pressed) {
            data->right_control_pressed = pressed;
        }
        else {
            return false;
        }
    }
    else if (keyCode == CpdKeyCode_LeftAlt) {
        if (data->left_alt_pressed != pressed) {
            data->left_alt_pressed = pressed;
        }
        else {
            return false;
        }
    }
    else if (keyCode == CpdKeyCode_RightAlt) {
        if (data->right_alt_pressed != pressed) {
            data->right_alt_pressed = pressed;
        }
        else {
            return false;
        }
    }
    else if (keyCode == CpdKeyCode_Invalid) {
        return false;
    }

    return true;
}

LRESULT window_procedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SYSCOMMAND:
            switch (GET_SC_WPARAM(wParam)) {
                case SC_CLOSE:
                    goto wndProc_close;

                case SC_MAXIMIZE:
                case SC_RESTORE:
                    DefWindowProcW(hWnd, uMsg, wParam, lParam);
                    goto wndProc_sizing;
            }
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        case WM_SIZE:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);
            if (data != 0) {
                data->minimized = (wParam & SIZE_MINIMIZED) != 0;

                RECT rectangle = { 0, 0, 0, 0 };
                GetClientRect(hWnd, &rectangle);
                data->size = (CpdSize) {
                    .width = (unsigned short)(rectangle.right - rectangle.left),
                    .height = (unsigned short)(rectangle.bottom - rectangle.top)
                };
            }
        }
        return 0;

        case WM_CLOSE:
        wndProc_close:
            close_window((CpdWindow)hWnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZING:
        wndProc_sizing:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);

            data->resized = true;
        }
        return 0;

        case WM_CHAR:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);

            if (data->input_mode != CpdInputMode_Text) {
                return 0;
            }

            uint32_t character = 0;
            int length = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)&wParam, 1, (char*)&character, sizeof(uint32_t), 0, 0);

            if (length == 0) {
                return 1;
            }

            return add_char_input_to_queue(data, character, (uint32_t)length);
        }

        case WM_MOUSEMOVE:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);

            int32_t x = GET_X_LPARAM(lParam);
            int32_t y = GET_Y_LPARAM(lParam);

            add_mouse_move_to_queue(data, x, y);

            data->mouse_x = x;
            data->mouse_y = y;
        }
        return 0;

        case WM_MOUSEWHEEL:
            return add_mouse_scroll_to_queue(GET_EXTRA_DATA(hWnd), -GET_WHEEL_DELTA_WPARAM(wParam), 0);

        case WM_MOUSEHWHEEL:
            return add_mouse_scroll_to_queue(GET_EXTRA_DATA(hWnd), 0, -GET_WHEEL_DELTA_WPARAM(wParam));

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return add_mouse_button_press_to_queue(GET_EXTRA_DATA(hWnd), CpdMouseButtonType_Left, (wParam & MK_LBUTTON) != 0);

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return add_mouse_button_press_to_queue(GET_EXTRA_DATA(hWnd), CpdMouseButtonType_Middle, (wParam & MK_MBUTTON) != 0);

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return add_mouse_button_press_to_queue(GET_EXTRA_DATA(hWnd), CpdMouseButtonType_Right, (wParam & MK_RBUTTON) != 0);

        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            if ((HIWORD(wParam) & XBUTTON1) != 0) {
                return add_mouse_button_press_to_queue(GET_EXTRA_DATA(hWnd), CpdMouseButtonType_Extra1, (wParam & MK_XBUTTON1) != 0);
            }
            else if ((HIWORD(wParam) & XBUTTON2) != 0) {
                return add_mouse_button_press_to_queue(GET_EXTRA_DATA(hWnd), CpdMouseButtonType_Extra2, (wParam & MK_XBUTTON2) != 0);
            }
            return 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);

            if (data->use_raw_input) {
                return 0;
            }

            bool wasPressed = (HIWORD(lParam) & KF_REPEAT) != 0;
            bool isPressed = (HIWORD(lParam) & KF_UP) == 0;

            bool is_modifier_key = wParam == VK_SHIFT || wParam == VK_CONTROL || wParam == VK_MENU;

            // Ignore repeating events for non-modifier keys
            if (wasPressed == isPressed && !is_modifier_key) {
                return 0;
            }

            USHORT scan_code = LOBYTE(HIWORD(lParam));
            if ((HIWORD(lParam) & KF_EXTENDED) != 0) {
                scan_code += 0xE000;
            }

            CpdKeyCode keyCode = virtual_key_to_key_code((USHORT)wParam, scan_code);
            if (!check_key_code(data, keyCode, isPressed)) {
                return 0;
            }

            if (wParam == VK_SHIFT) {
                set_key_modifier(data, CpdInputModifierKey_Shift, HIBYTE(GetKeyState(VK_LSHIFT)) || HIBYTE(GetKeyState(VK_RSHIFT)));
            }
            else if (wParam == VK_CONTROL) {
                set_key_modifier(data, CpdInputModifierKey_Control, HIBYTE(GetKeyState(VK_LCONTROL)) || HIBYTE(GetKeyState(VK_RCONTROL)));
            }
            else if (wParam == VK_MENU) {
                set_key_modifier(data, CpdInputModifierKey_Alt, HIBYTE(GetKeyState(VK_LMENU)) || HIBYTE(GetKeyState(VK_RMENU)));
            }

            LRESULT result = add_button_press_to_queue(data, isPressed, keyCode);
            if (result != 0) {
                return result;
            }
        }
        return 0;

        case WM_INPUT:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);
            HRAWINPUT raw_input_handle = (HRAWINPUT)lParam;

            UINT data_size = sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD);
            RAWINPUT raw_input;
            if (GetRawInputData(raw_input_handle, RID_INPUT, &raw_input, &data_size, sizeof(RAWINPUTHEADER)) != data_size) {
                return 1;
            }

            if (raw_input.header.dwType == RIM_TYPEKEYBOARD && raw_input.data.keyboard.MakeCode != KEYBOARD_OVERRUN_MAKE_CODE && raw_input.data.keyboard.VKey < UCHAR_MAX) {
                uint16_t scan_code = raw_input.data.keyboard.MakeCode & 0x7F;

                if (scan_code == 0) {
                    scan_code = LOWORD(MapVirtualKeyW(raw_input.data.keyboard.VKey, MAPVK_VK_TO_VSC_EX));
                }

                if ((raw_input.data.keyboard.Flags & RI_KEY_E0) != 0) {
                    scan_code = MAKEWORD(scan_code, 0xE0);
                }
                else if ((raw_input.data.keyboard.Flags & RI_KEY_E1) != 0) {
                    scan_code = MAKEWORD(scan_code, 0xE1);
                }

                bool pressed = (raw_input.data.keyboard.Flags & RI_KEY_BREAK) == 0;

                for (uint32_t i = 0; i < data->keyboard_presses_size; i++) {
                    if (data->keyboard_presses[i].scan_code != scan_code) {
                        continue;
                    }

                    if (data->keyboard_presses[i].pressed == pressed) {
                        return 0;
                    }
                }

                CpdKeyCode keyCode = virtual_key_to_key_code(raw_input.data.keyboard.VKey, scan_code);
                if (!check_key_code(data, keyCode, pressed)) {
                    return 0;
                }

                if (raw_input.data.keyboard.VKey == VK_SHIFT) {
                    WORD vkey = LOWORD(MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX));
                    bool modifier_pressed = pressed || HIBYTE(GetKeyState(vkey == VK_RSHIFT ? VK_LSHIFT : VK_RSHIFT));
                    
                    set_key_modifier(data, CpdInputModifierKey_Shift, modifier_pressed);
                }
                else if (raw_input.data.keyboard.VKey == VK_CONTROL) {
                    WORD vkey = LOWORD(MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX));
                    bool modifier_pressed = pressed || HIBYTE(GetKeyState(vkey == VK_RCONTROL ? VK_LCONTROL : VK_RCONTROL));
                    
                    set_key_modifier(data, CpdInputModifierKey_Control, modifier_pressed);
                }
                else if (raw_input.data.keyboard.VKey == VK_MENU) {
                    WORD vkey = LOWORD(MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX));
                    bool modifier_pressed = pressed || HIBYTE(GetKeyState(vkey == VK_RMENU ? VK_LMENU : VK_RMENU));
                    
                    set_key_modifier(data, CpdInputModifierKey_Alt, modifier_pressed);
                }

                LRESULT result = add_button_press_to_queue(data, pressed, keyCode);
                if (result != 0) {
                    return result;
                }

                if (!set_keyboard_key_press(data, scan_code, pressed)) {
                    return 1;
                }
            }
        }
        return 0;

        case WM_ACTIVATE:
        {
            WindowExtraData* data = GET_EXTRA_DATA(hWnd);

            if (data == 0) {
                return 1;
            }

            data->focused = LOWORD(wParam) != WA_INACTIVE;

            if (data->focused) {
                if (data->use_raw_input) {
                    data->use_raw_input = register_raw_input(hWnd);
                }

#ifdef CPD_IMGUI_AVAILABLE
                ImGui_SetCurrentContext(data->imgui_context);
#endif
            }
        }
        return 0;

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
