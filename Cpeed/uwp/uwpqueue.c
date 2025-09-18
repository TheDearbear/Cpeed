#include "uwpmain.h"

bool resize_input_queue_if_need(CpdUWPWindow* uwp_window, uint32_t new_events) {
    uint32_t new_size = uwp_window->input_queue_size + new_events;

    if (uwp_window->input_queue_max_size >= new_size) {
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

    for (uint32_t i = 0; i < uwp_window->input_queue_size; i++) {
        new_input_queue[i] = uwp_window->input_queue[i];
    }

    CpdInputEvent* old_input_queue = uwp_window->input_queue;

    uwp_window->input_queue = new_input_queue;
    uwp_window->input_queue_max_size = new_max_size;
    uwp_window->resize_swap_queue = true;

    free(old_input_queue);

    return true;
}

bool add_button_press_to_queue(CpdUWPWindow* uwp_window, CpdKeyCode keyCode, bool pressed) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_ButtonPress,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.button_press.key_code = keyCode,
        .data.button_press.pressed = pressed
    };

    return true;
}

bool add_char_input_to_queue(CpdUWPWindow* uwp_window, uint32_t character, uint32_t length) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_CharInput,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.char_input.character = character,
        .data.char_input.length = length
    };

    return true;
}

bool add_mouse_button_press_to_queue(CpdUWPWindow* uwp_window, CpdMouseButtonType button, bool pressed) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseButtonPress,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_button_press.button = button,
        .data.mouse_button_press.pressed = pressed
    };

    return true;
}

bool add_mouse_move_to_queue(CpdUWPWindow* uwp_window, int16_t x_pos, int16_t y_pos) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    int16_t delta_x = 0;
    int16_t delta_y = 0;

    if (uwp_window->first_mouse_event) {
        uwp_window->first_mouse_event = false;
    }
    else {
        delta_x = x_pos - uwp_window->mouse_x;
        delta_y = y_pos - uwp_window->mouse_y;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseMove,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_move.x_pos = x_pos,
        .data.mouse_move.y_pos = y_pos,
        .data.mouse_move.x_move = delta_x,
        .data.mouse_move.y_move = delta_y
    };

    return true;
}

bool add_mouse_scroll_to_queue(CpdUWPWindow* uwp_window, int32_t vertical, int32_t horizontal) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseScroll,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.mouse_scroll.vertical_scroll = vertical,
        .data.mouse_scroll.horizontal_scroll = horizontal
    };

    return true;
}

bool add_gamepad_connect_to_queue(CpdUWPWindow* uwp_window, CpdGamepadConnectStatus status, uint16_t index) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent){
        .type = CpdInputEventType_GamepadConnect,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_connect.status = status,
        .data.gamepad_connect.gamepad_index = index
    };

    return true;
}

bool add_gamepad_stick_to_queue(CpdUWPWindow* uwp_window, CpdGamepadStick stick, uint16_t index) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_GamepadStick,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_stick.stick = stick,
        .data.gamepad_stick.gamepad_index = index
    };

    return true;
}

bool add_gamepad_trigger_to_queue(CpdUWPWindow* uwp_window, CpdGamepadTrigger trigger, uint16_t index) {
    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_GamepadTrigger,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_trigger.trigger = trigger,
        .data.gamepad_trigger.gamepad_index = index
    };

    return true;
}

bool add_gamepad_button_press_to_queue(CpdUWPWindow* uwp_window, CpdGamepadButtonType button, uint16_t index, bool pressed) {
    printf("Button (%d): %d -> %d\n", index, button, pressed);

    if (!resize_input_queue_if_need(uwp_window, 1)) {
        return false;
    }

    uwp_window->input_queue[uwp_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_GamepadButtonPress,
        .modifiers = uwp_window->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_button_press.button = button,
        .data.gamepad_button_press.gamepad_index = index,
        .data.gamepad_button_press.pressed = pressed
    };

    return true;
}
