#include <malloc.h>

#include <Cpeed/platform/input/gamepad.h>
#include <Cpeed/platform/frame.h>
#include <Cpeed/platform/window.h>
#include <Cpeed/platform/logging.h>
#include <Cpeed/platform.h>

#include "wininput.h"
#include "winmain.h"

CpdWindow create_window(const CpdWindowInfo* info) {
    WindowExtraData* data = (WindowExtraData*)malloc(sizeof(WindowExtraData));
    CpdInputEvent* input_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    CpdInputEvent* input_swap_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    CpdKeyboardKey* keyboard_presses = (CpdKeyboardKey*)malloc(KEYBOARD_PRESSES_BASE_SIZE * sizeof(CpdKeyboardKey));

    if (data == 0 || input_queue == 0 || input_swap_queue == 0 || keyboard_presses == 0) {
        free(data);
        free(input_queue);
        free(input_swap_queue);
        free(keyboard_presses);
        return 0;
    }

    HWND hWnd = CreateWindowExA(0, MAKEINTATOM(g_window_class), info->title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, info->size.width, info->size.height,
        NULL, NULL, GetModuleHandleW(NULL), NULL);
    
    if (hWnd == 0) {
        free(data);
        free(input_queue);
        free(input_swap_queue);
        free(keyboard_presses);
        return 0;
    }

#ifdef CPD_IMGUI_AVAILABLE
    data->imgui_context = ImGui_CreateContext(0);
    ImGui_SetCurrentContext(data->imgui_context);
#endif

    data->input_queue = input_queue;
    data->input_swap_queue = input_swap_queue;
    data->input_queue_size = 0;
    data->input_swap_queue_size = 0;
    data->input_queue_max_size = INPUT_QUEUE_BASE_SIZE;
    data->input_mode = info->input_mode;
    data->current_key_modifiers = CpdInputModifierKey_None;

    RECT rectangle = { 0, 0, 0, 0 };
    GetClientRect(hWnd, &rectangle);

    data->size = (CpdSize) {
        .width = (unsigned short)(rectangle.right - rectangle.left),
        .height = (unsigned short)(rectangle.bottom - rectangle.top),
    };
    data->mouse_x = 0;
    data->mouse_y = 0;

    data->keyboard_presses = keyboard_presses;
    data->keyboard_presses_size = 0;
    data->keyboard_presses_max_size = KEYBOARD_PRESSES_BASE_SIZE;

    data->layers = 0;

    data->resized = false;
    data->should_close = false;
    data->minimized = false;
    data->resize_swap_queue = false;
    data->first_mouse_event = true;
    data->focused = true;
    data->use_raw_input = true;

    data->left_shift_pressed = false;
    data->right_shift_pressed = false;
    data->left_control_pressed = false;
    data->right_control_pressed = false;
    data->left_alt_pressed = false;
    data->right_alt_pressed = false;
    
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)data);

    uint16_t count = get_gamepad_count((CpdWindow)hWnd);
    if (!resize_input_queue_if_need(data, count)) {
        destroy_window((CpdWindow)hWnd);
        return 0;
    }

    for (uint16_t i = 0; i < count; i++) {
        data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
            .type = CpdInputEventType_GamepadConnect,
            .modifiers = data->current_key_modifiers,
            .time = get_clock_usec(),
            .data.gamepad_connect.status = CpdGamepadConnectStatus_Connected,
            .data.gamepad_connect.gamepad_index = i
        };
    }

    if (data->use_raw_input) {
        data->use_raw_input = register_raw_input(hWnd);
    }

    return (CpdWindow)hWnd;
}

static bool remove_frame_layers_loop(void* context, CpdFrameLayer* layer) {
    CpdWindow window = (CpdWindow)context;

    remove_frame_layer(window, layer->handle);

    return true;
}

void destroy_window(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    loop_frame_layers(window, remove_frame_layers_loop, (void*)window);

#ifdef CPD_IMGUI_AVAILABLE
    ImGui_DestroyContext(data->imgui_context);
#endif

    cleanup_input_queue(data->input_queue, data->input_queue_size);
    cleanup_input_queue(data->input_swap_queue, data->input_swap_queue_size);

    free(data->input_queue);
    free(data->input_swap_queue);
    free(data->keyboard_presses);
    free(data);

    DestroyWindow((HWND)window);
}

void close_window(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    data->should_close = true;
}

static bool add_gamepad_trigger_to_queue(WindowExtraData* data, CpdGamepadTrigger trigger, uint16_t index) {
    if (!resize_input_queue_if_need(data, 1)) {
        return false;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_GamepadTrigger,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_trigger.trigger = trigger,
        .data.gamepad_trigger.gamepad_index = index
    };

    return true;
}

static bool add_gamepad_stick_to_queue(WindowExtraData* data, CpdGamepadStick stick, uint16_t index) {
    if (!resize_input_queue_if_need(data, 1)) {
        return false;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_GamepadStick,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_stick.stick = stick,
        .data.gamepad_stick.gamepad_index = index
    };

    return true;
}

static bool add_gamepad_button_press_to_queue(WindowExtraData* data, CpdGamepadButtonType button, uint16_t index, bool pressed) {
    if (!resize_input_queue_if_need(data, 1)) {
        return false;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_GamepadButtonPress,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_button_press.button = button,
        .data.gamepad_button_press.gamepad_index = index,
        .data.gamepad_button_press.pressed = pressed
    };

    return true;
}

static inline void check_gamepad_button(WindowExtraData* data,
    GameInputGamepadState* current, GameInputGamepadState* last_used_state,
    GameInputGamepadButtons from_button, CpdGamepadButtonType to_button,
    uint16_t index
) {
    if ((current->buttons & from_button) != (last_used_state->buttons & from_button)) {
        bool pressed = (current->buttons & from_button) != 0;

        if (add_gamepad_button_press_to_queue(data, to_button, index, pressed)) {
            if (pressed) {
                last_used_state->buttons |= from_button;
            }
            else {
                last_used_state->buttons &= ~from_button;
            }
        }
    }
}

static void poll_gamepads(WindowExtraData* data) {
    static float x = 0.0f;
    static float y = 0.0f;

    if (g_gamepads == 0 || !data->focused) {
        return;
    }

    uint16_t index = 0;
    for (CpdGamepad* gamepad = g_gamepads; gamepad != 0; index++) {
        IGameInputReading* reading;
        HRESULT result = IGameInput_GetCurrentReading(g_game_input, GameInputKindGamepad, gamepad->device, &reading);

        if (SUCCEEDED(result)) {
            GameInputGamepadState state;
            GameInputGamepadState* last_used_state = &gamepad->last_used_state;

            if (IGameInputReading_GetGamepadState(reading, &state)) {
                if (state.leftTrigger != last_used_state->leftTrigger && add_gamepad_trigger_to_queue(data, CpdGamepadTrigger_Left, index)) {
                    last_used_state->leftTrigger = state.leftTrigger;
                }

                if (state.rightTrigger != last_used_state->rightTrigger && add_gamepad_trigger_to_queue(data, CpdGamepadTrigger_Right, index)) {
                    last_used_state->rightTrigger = state.rightTrigger;
                }

                if ((state.leftThumbstickX != last_used_state->leftThumbstickX || state.leftThumbstickY != last_used_state->leftThumbstickY) &&
                    add_gamepad_stick_to_queue(data, CpdGamepadStick_Left, index)
                ) {
                    last_used_state->leftThumbstickX = state.leftThumbstickX;
                    last_used_state->leftThumbstickY = state.leftThumbstickY;
                }

                if ((state.rightThumbstickX != last_used_state->rightThumbstickX || state.rightThumbstickY != last_used_state->rightThumbstickY) &&
                    add_gamepad_stick_to_queue(data, CpdGamepadStick_Right, index)
                ) {
                    last_used_state->rightThumbstickX = state.rightThumbstickX;
                    last_used_state->rightThumbstickY = state.rightThumbstickY;
                }

                check_gamepad_button(data, &state, last_used_state, GameInputGamepadMenu, CpdGamepadButtonType_Menu, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadView, CpdGamepadButtonType_View, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadA, CpdGamepadButtonType_A, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadB, CpdGamepadButtonType_B, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadX, CpdGamepadButtonType_X, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadY, CpdGamepadButtonType_Y, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadDPadUp, CpdGamepadButtonType_DPadUp, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadDPadDown, CpdGamepadButtonType_DPadDown, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadDPadLeft, CpdGamepadButtonType_DPadLeft, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadDPadRight, CpdGamepadButtonType_DPadRight, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadLeftShoulder, CpdGamepadButtonType_ShoulderLeft, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadRightShoulder, CpdGamepadButtonType_ShoulderRight, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadLeftThumbstick, CpdGamepadButtonType_StickLeft, index);
                check_gamepad_button(data, &state, last_used_state, GameInputGamepadRightThumbstick, CpdGamepadButtonType_StickRight, index);
            }

            IGameInputReading_Release(reading);
        }

        gamepad = gamepad->next;
    }
}

bool poll_window(CpdWindow window) {
    MSG msg;
    while (PeekMessageW(&msg, (HWND)window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    if (data == 0) {
        DWORD err = GetLastError();
        log_error("Unable to read window state. Error code: %d\n", err);
        return true;
    }

    poll_gamepads(data);

    return data->should_close;
}

CpdSize window_size(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return data->size;
}

bool window_resized(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);
    
    bool resized = data->resized;
    data->resized = false;

    return resized;
}

bool window_present_allowed(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return !data->minimized;
}

ImGuiContext* get_imgui_context(CpdWindow window) {
#ifdef CPD_IMGUI_AVAILABLE
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return data->imgui_context;
#else
    return 0;
#endif
}

bool multiple_windows_supported() {
    return true;
}

bool windowed_mode_supported() {
    return true;
}
