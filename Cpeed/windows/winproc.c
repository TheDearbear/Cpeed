#include <malloc.h>
#include <windowsx.h>

#include "../platform/window.h"
#include "winmain.h"
#include "winproc.h"

static bool resize_input_queue_if_need(WindowExtraData* data, uint32_t new_events) {
    uint32_t new_size = data->input_queue_size + new_events;

    if (data->input_queue_max_size >= new_size) {
        return true;
    }

    uint32_t new_max_size = new_size;
    uint32_t remainder = new_max_size % INPUT_QUEUE_SIZE_STEP;

    if (remainder != 0) {
        new_max_size += INPUT_QUEUE_SIZE_STEP - remainder;
    }

    CpdInputEvent* new_input_queue = (CpdInputEvent*)malloc(new_max_size * sizeof(CpdInputEvent));
    if (new_input_queue == 0) {
        return false;
    }

    for (uint32_t i = 0; i < data->input_queue_size; i++) {
        new_input_queue[i] = data->input_queue[i];
    }

    CpdInputEvent* old_input_queue = data->input_queue;

    data->input_queue = new_input_queue;
    data->input_queue_max_size = new_max_size;
    data->resize_swap_queue = true;

    free(old_input_queue);

    return true;
}


static LRESULT add_button_press_to_queue(WindowExtraData* data, LPARAM lParam, CpdKeyCode keyCode) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent){
        .type = CpdInputEventType_ButtonPress,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.button_press.key_code = keyCode,
        .data.button_press.pressed = (HIWORD(lParam) & KF_UP) == 0
    };

    return 0;
}

static LRESULT add_mouse_scroll_to_queue(WindowExtraData* data, int32_t vertical, int32_t horizontal) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent){
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

    data->input_queue[data->input_queue_size++] = (CpdInputEvent){
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

    data->input_queue[data->input_queue_size++] = (CpdInputEvent){
        .type = CpdInputEventType_MouseButtonPress,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_button_press.button = button,
        .data.mouse_button_press.pressed = pressed
    };

    return 0;
}

static LRESULT add_mouse_move_to_queue(WindowExtraData* data, int16_t new_x, int16_t new_y) {
    if (!resize_input_queue_if_need(data, 1)) {
        return 1;
    }

    int16_t delta_x = 0;
    int16_t delta_y = 0;

    if (data->first_mouse_event) {
        data->first_mouse_event = false;
    }
    else {
        delta_x = new_x - data->mouse_x;
        delta_y = new_y - data->mouse_y;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent){
        .type = CpdInputEventType_MouseMove,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_move.x_pos = new_x,
        .data.mouse_move.y_pos = new_y,
        .data.mouse_move.x_move = delta_x,
        .data.mouse_move.y_move = delta_y
    };

    return 0;
}

static void set_key_modifier(WindowExtraData* data, LPARAM lParam, CpdInputModifierKeyFlags modifier) {
    if ((HIWORD(lParam) & KF_UP) == 0) {
        data->current_key_modifiers |= modifier;
    }
    else {
        data->current_key_modifiers &= ~modifier;
    }
}

static LRESULT add_modifier_button_press_to_queue(WindowExtraData* data, LPARAM lParam, CpdKeyCode keyCode, CpdInputModifierKeyFlags modifier) {
    LRESULT result = add_button_press_to_queue(data, lParam, keyCode);

    set_key_modifier(data, lParam, modifier);

    return result;
}

static CpdKeyCode virtual_key_to_key_code(WPARAM wParam) {
    if (wParam >= VK_F1 && wParam <= VK_F24) {
        return wParam - (VK_F1 - CpdKeyCode_F1);
    }

    if (wParam >= 'A' && wParam <= 'Z') {
        return wParam - ('A' - CpdKeyCode_A);
    }

    if (wParam >= '0' && wParam <= '9') {
        return wParam - ('0' - CpdKeyCode_0);
    }

    if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) {
        return wParam - (VK_NUMPAD0 - CpdKeyCode_Numpad0);
    }

    switch (wParam) {
    case VK_BACK:      return CpdKeyCode_Backspace;
    case VK_TAB:       return CpdKeyCode_Tab;
    case VK_RETURN:    return CpdKeyCode_Enter;
    case VK_SHIFT:     return CpdKeyCode_Shift;
    case VK_CONTROL:   return CpdKeyCode_Control;
    case VK_MENU:      return CpdKeyCode_Alt;
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
            data->minimized = wParam & SIZE_MINIMIZED != 0;
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

        int16_t x = GET_X_LPARAM(lParam);
        int16_t y = GET_Y_LPARAM(lParam);

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

        bool wasPressed = (HIWORD(lParam) & KF_REPEAT) != 0;
        bool isPressed = (HIWORD(lParam) & KF_UP) == 0;

        // Ignore repeating events
        if (wasPressed == isPressed) {
            return 0;
        }

        switch (wParam) {
        case VK_SHIFT:
            return add_modifier_button_press_to_queue(data, lParam, CpdKeyCode_Shift, CpdInputModifierKey_Shift);

        case VK_CONTROL:
            return add_modifier_button_press_to_queue(data, lParam, CpdKeyCode_Control, CpdInputModifierKey_Control);

        case VK_MENU:
            return add_modifier_button_press_to_queue(data, lParam, CpdKeyCode_Alt, CpdInputModifierKey_Alt);

        default:
        {
            CpdKeyCode keyCode = virtual_key_to_key_code(wParam);
            if (keyCode != CpdKeyCode_Invalid) {
                return add_button_press_to_queue(data, lParam, keyCode);
            }
        }
        break;
        }
    }
    return 0;

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
