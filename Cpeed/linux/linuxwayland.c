#include <linux/input-event-codes.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-names.h>

#include "../platform.h"
#include "linuxmain.h"
#include "linuxwayland.h"
#include "linuxwindowlist.h"

CpdWaylandWindow* g_current_pointer_focus;
CpdWaylandWindow* g_current_keyboard_focus;

#ifndef WL_KEYBOARD_KEY_STATE_REPEATED
#define WL_KEYBOARD_KEY_STATE_REPEATED 2
#endif

static CpdInputEvent g_current_pointer_event = {
    .type = CpdInputEventType_None,
    .modifiers = CpdInputModifierKey_None
};

struct wl_compositor* g_compositor;
struct wl_display* g_display;
struct wl_seat* g_seat;
struct xdg_wm_base* g_wm_base;

struct xkb_context* g_xkb_context;

static struct wl_pointer* g_pointer;
static CpdWaylandKeyboard g_keyboard;

static bool resize_input_queue_if_need(CpdWaylandWindow* window, uint32_t new_events);
static void queue_mouse_move_event(CpdWaylandWindow* wl_window, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void add_mouse_button_event(CpdWaylandWindow* wl_window, CpdMouseButtonType type, bool pressed);
static void add_button_press_event(CpdWaylandWindow* wl_window, CpdKeyCode keyCode, xkb_keycode_t xkbKeyCode, bool pressed);
static void add_char_input_event(CpdWaylandWindow* wl_window, uint32_t character, uint32_t length);
static void add_mouse_scroll_event(CpdWaylandWindow* wl_window, enum wl_pointer_axis axis, int32_t value);
static void add_current_mouse_event(CpdWaylandWindow* wl_window);

static void sync_keyboard_modifiers();
static CpdKeyCode map_key_code(uint32_t keyCode);

static CpdPressedButton* find_entry_by_key_code(CpdKeyCode keyCode);
static CpdPressedButton* find_entry_before(CpdPressedButton* entry);

static void noop() {}

void destroy_pointer() {
    if (g_pointer != 0) {
        wl_pointer_destroy(g_pointer);
        g_pointer = 0;
    }
}

static void destroy_pressed_buttons() {
    CpdPressedButton* current = g_keyboard.pressed_buttons;
    while (current != 0) {
        CpdPressedButton* next = current->next;
        free(current);
        current = next;
    }

    g_keyboard.pressed_buttons = 0;
}

void destroy_keyboard() {
    if (g_keyboard.state != 0) {
        xkb_state_unref(g_keyboard.state);
        g_keyboard.state = 0;
    }

    if (g_keyboard.keymap != 0) {
        xkb_keymap_unref(g_keyboard.keymap);
        g_keyboard.keymap = 0;
    }

    if (g_keyboard.keyboard != 0) {
        wl_keyboard_destroy(g_keyboard.keyboard);
        g_keyboard.keyboard = 0;
    }

    destroy_pressed_buttons();

    g_keyboard.modifiers = CpdInputModifierKey_None;

    g_keyboard.mods_depressed = 0;
    g_keyboard.mods_latched = 0;
    g_keyboard.mods_locked = 0;
    g_keyboard.group = 0;

    g_keyboard.repeat_delay = 0;
    g_keyboard.repeat_rate = 0;
}


// =================
// |    POINTER    |
// =================


static void pointer_enter(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    CpdWaylandWindow* wl_window = find_window_by_surface(surface);
    if (wl_window == 0) {
        return;
    }

    g_current_pointer_focus = wl_window;

    queue_mouse_move_event(wl_window, surface_x, surface_y);
}

static void pointer_leave(void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface) {
    g_current_pointer_focus = 0;
}

static void pointer_motion(void* data, struct wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    if (g_current_pointer_focus == 0) {
        return;
    }

    queue_mouse_move_event(g_current_pointer_focus, surface_x, surface_y);
}

static void pointer_button(void* data, struct wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    if (g_current_pointer_focus == 0) {
        return;
    }

    if (button == BTN_LEFT) {
        add_mouse_button_event(g_current_pointer_focus, CpdMouseButtonType_Left, state != 0);
    }
    else if (button == BTN_RIGHT) {
        add_mouse_button_event(g_current_pointer_focus, CpdMouseButtonType_Right, state != 0);
    }
    else if (button == BTN_MIDDLE) {
        add_mouse_button_event(g_current_pointer_focus, CpdMouseButtonType_Middle, state != 0);
    }
    else if (button == BTN_SIDE) {
        add_mouse_button_event(g_current_pointer_focus, CpdMouseButtonType_Extra1, state != 0);
    }
    else if (button == BTN_EXTRA) {
        add_mouse_button_event(g_current_pointer_focus, CpdMouseButtonType_Extra2, state != 0);
    }
}

static void pointer_frame(void* data, struct wl_pointer* wl_pointer) {
    if (g_current_pointer_focus == 0) {
        return;
    }

    if (g_current_pointer_event.type == CpdInputEventType_None) {
        return;
    }

    add_current_mouse_event(g_current_pointer_focus);
}

static void pointer_axis_scroll(void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t value) {
    if (g_current_pointer_focus == 0) {
        return;
    }

    uint32_t pointer_version = wl_pointer_get_version(wl_pointer);

    if (pointer_version < WL_POINTER_AXIS_VALUE120_SINCE_VERSION) {
        value *= 120;
    }

    add_mouse_scroll_event(g_current_keyboard_focus, (enum wl_pointer_axis)axis, value);
}

static struct wl_pointer_listener pointer_listener_new = (struct wl_pointer_listener) {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = noop,
    .frame = pointer_frame,
    .axis_source = noop,
    .axis_stop = noop,
    .axis_discrete = 0,
    .axis_value120 = pointer_axis_scroll,
    .axis_relative_direction = noop
};

static struct wl_pointer_listener pointer_listener_old = (struct wl_pointer_listener) {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = noop,
    .frame = pointer_frame,
    .axis_source = noop,
    .axis_stop = noop,
    .axis_discrete = pointer_axis_scroll
};


// ================
// |   KEYBOARD   |
// ================


static void keyboard_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size) {
    if (g_keyboard.state != 0) {
        xkb_state_unref(g_keyboard.state);
        g_keyboard.state = 0;
    }

    if (g_keyboard.keymap != 0) {
        xkb_keymap_unref(g_keyboard.keymap);
        g_keyboard.keymap = 0;
    }

    if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        const char* keymap = (const char*)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (keymap == MAP_FAILED) {
            printf("Unable to memory map keymap\n");
            close(fd);
            return;
        }

        g_keyboard.keymap = xkb_keymap_new_from_buffer(g_xkb_context, keymap, size, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

        munmap((void*)keymap, size);
    }
    else if (format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP) {
        g_keyboard.keymap = xkb_keymap_new_from_names(g_xkb_context, 0, XKB_KEYMAP_COMPILE_NO_FLAGS);
    }
    else {
        printf("Unknown keymap format (%d)\n", format);
    }

    close(fd);

    if (g_keyboard.keymap == 0) {
        return;
    }

    g_keyboard.state = xkb_state_new(g_keyboard.keymap);
}

static void keyboard_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys) {
    CpdWaylandWindow* wl_window = find_window_by_surface(surface);
    if (wl_window == 0) {
        return;
    }

    g_current_keyboard_focus = wl_window;
}

static void keyboard_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface) {
    g_current_keyboard_focus = 0;
}

static void keyboard_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    if (g_keyboard.state == 0) {
        return;
    }

    // Process only "released" and "pressed"
    if (state < WL_KEYBOARD_KEY_STATE_REPEATED) {
        bool pressed = state != WL_KEYBOARD_KEY_STATE_RELEASED;
        enum xkb_key_direction direction = pressed ? XKB_KEY_DOWN : XKB_KEY_UP;

        xkb_state_update_key(g_keyboard.state, (xkb_keycode_t)(key + 8), direction);

        CpdKeyCode keyCode = map_key_code(key);
        if (keyCode != CpdKeyCode_Invalid) {
            add_button_press_event(g_current_keyboard_focus, keyCode, (xkb_keycode_t)(key + 8), pressed);
        }
    }

    bool isCharInput = state == WL_KEYBOARD_KEY_STATE_PRESSED || state == WL_KEYBOARD_KEY_STATE_REPEATED;

    if (g_current_keyboard_focus->input_mode == CpdInputMode_Text && isCharInput) {
        uint64_t character = 0;
        uint32_t length = xkb_state_key_get_utf8(g_keyboard.state, (xkb_keycode_t)(key + 8), (char*)&character, sizeof(uint32_t));

        if (length == 0) {
            return;
        }

        add_char_input_event(g_current_keyboard_focus, (uint32_t)character, length);
    }
}

static void keyboard_modifiers(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial,
    uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group
) {
    g_keyboard.mods_depressed = mods_depressed;
    g_keyboard.mods_latched = mods_latched;
    g_keyboard.mods_locked = mods_locked;
    g_keyboard.group = group;

    sync_keyboard_modifiers();
}

static void sync_keyboard_modifiers() {
    if (g_keyboard.state == 0) {
        return;
    }

    xkb_state_update_mask(g_keyboard.state,
        (xkb_mod_mask_t)g_keyboard.mods_depressed, (xkb_mod_mask_t)g_keyboard.mods_latched, (xkb_mod_mask_t)g_keyboard.mods_locked,
        (xkb_layout_index_t)g_keyboard.group, (xkb_layout_index_t)g_keyboard.group, (xkb_layout_index_t)g_keyboard.group);

    g_keyboard.modifiers = CpdInputModifierKey_None;

    if (xkb_state_mod_name_is_active(g_keyboard.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_DEPRESSED)) {
        g_keyboard.modifiers |= CpdInputModifierKey_Shift;
    }

    if (xkb_state_mod_name_is_active(g_keyboard.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_DEPRESSED)) {
        g_keyboard.modifiers |= CpdInputModifierKey_Control;
    }

    if (xkb_state_mod_name_is_active(g_keyboard.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_DEPRESSED)) {
        g_keyboard.modifiers |= CpdInputModifierKey_Alt;
    }
}

static void keyboard_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay) {
    g_keyboard.repeat_rate = (uint32_t)rate;
    g_keyboard.repeat_delay = (uint32_t)delay;
}

static struct wl_keyboard_listener keyboard_listener = (struct wl_keyboard_listener) {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info
};


// ================
// |     SEAT     |
// ================


static void seat_capabilities(void* data, struct wl_seat* wl_seat, uint32_t capabilities) {
    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0) {
        g_pointer = wl_seat_get_pointer(wl_seat);

        struct wl_pointer_listener* listener = &pointer_listener_old;

        if (wl_pointer_get_version(g_pointer) >= WL_POINTER_AXIS_VALUE120_SINCE_VERSION) {
            listener = &pointer_listener_new;
        }

        wl_pointer_add_listener(g_pointer, listener, 0);
    }
    else {
        destroy_pointer();
    }

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0) {
        g_keyboard.keyboard = wl_seat_get_keyboard(wl_seat);

        wl_keyboard_add_listener(g_keyboard.keyboard, &keyboard_listener, 0);
    }
    else {
        destroy_keyboard();
    }
}

static void seat_name(void* data, struct wl_seat* wl_seat, const char* name) {
    printf("Wayland seat name: %s\n", name);
}

struct wl_seat_listener g_seat_listener = (struct wl_seat_listener) {
    .capabilities = seat_capabilities,
    .name = seat_name
};


// ===============
// |    QUEUE    |
// ===============


static bool resize_input_queue_if_need(CpdWaylandWindow* window, uint32_t new_events) {
    uint32_t new_size = window->input_queue_size + new_events;

    if (window->input_queue_max_size >= new_size) {
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

    for (uint32_t i = 0; i < window->input_queue_size; i++) {
        new_input_queue[i] = window->input_queue[i];
    }

    CpdInputEvent* old_input_queue = window->input_queue;

    window->input_queue = new_input_queue;
    window->input_queue_max_size = new_max_size;
    window->resize_swap_queue = true;

    free(old_input_queue);

    return true;
}

static void queue_mouse_move_event(CpdWaylandWindow* wl_window, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    int32_t position_x = wl_fixed_to_int(surface_x);
    int32_t position_y = wl_fixed_to_int(surface_y);

    int32_t delta_x = 0;
    int32_t delta_y = 0;

    if (wl_window->first_mouse_event) {
        wl_window->first_mouse_event = false;

        wl_window->mouse_x = position_x;
        wl_window->mouse_y = position_y;
    }
    else {
        delta_x = position_x - wl_window->mouse_x;
        delta_y = position_y - wl_window->mouse_y;
    }

    g_current_pointer_event.type = CpdInputEventType_MouseMove;
    g_current_pointer_event.modifiers = g_keyboard.modifiers;
    g_current_pointer_event.data.mouse_move.x_pos = position_x;
    g_current_pointer_event.data.mouse_move.y_pos = position_y;
    g_current_pointer_event.data.mouse_move.x_move = delta_x;
    g_current_pointer_event.data.mouse_move.y_move = delta_y;
}

static void add_mouse_button_event(CpdWaylandWindow* wl_window, CpdMouseButtonType type, bool pressed) {
    if (!resize_input_queue_if_need(wl_window, 1)) {
        return;
    }

    wl_window->input_queue[wl_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseButtonPress,
        .modifiers = g_keyboard.modifiers,
        .time = get_clock_usec(),
        .data.mouse_button_press.button = type,
        .data.mouse_button_press.pressed = pressed
    };
}

static void add_button_press_event(CpdWaylandWindow* wl_window, CpdKeyCode keyCode, xkb_keycode_t xkbKeyCode, bool pressed) {
    if (!resize_input_queue_if_need(wl_window, 1)) {
        return;
    }

    uint64_t moment = get_clock_usec();
    CpdPressedButton* entry = find_entry_by_key_code(keyCode);

    if (!pressed && entry != 0)
    {
        CpdPressedButton* previous = find_entry_before(entry);

        if (--entry->current_presses == 0) {
            if (previous == 0) {
                g_keyboard.pressed_buttons = entry->next;
            }
            else {
                previous->next = entry->next;
            }

            free(entry);
        }
    }
    else if (pressed && entry == 0) {
        entry = (CpdPressedButton*)malloc(sizeof(CpdPressedButton));
        if (entry != 0) {
            entry->next = 0;
            entry->time = moment;
            entry->key_code = keyCode;
            entry->xkb_key_code = xkbKeyCode;
            entry->current_presses = 1;

            if (g_keyboard.pressed_buttons == 0) {
                g_keyboard.pressed_buttons = entry;
            }
            else {
                CpdPressedButton* lastEntry = find_entry_before(0);
                lastEntry->next = entry;
            }
        }
    }
    else if (pressed) {
        entry->time = moment;
        entry->current_presses++;
        return;
    }

    wl_window->input_queue[wl_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_ButtonPress,
        .modifiers = g_keyboard.modifiers,
        .time = moment,
        .data.button_press.key_code = keyCode,
        .data.button_press.pressed = pressed
    };
}

static void add_char_input_event(CpdWaylandWindow* wl_window, uint32_t character, uint32_t length) {
    if (!resize_input_queue_if_need(wl_window, 1)) {
        return;
    }

    wl_window->input_queue[wl_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_CharInput,
        .modifiers = g_keyboard.modifiers,
        .time = get_clock_usec(),
        .data.char_input.character = character,
        .data.char_input.length = length
    };
}

static void add_mouse_scroll_event(CpdWaylandWindow* wl_window, enum wl_pointer_axis axis, int32_t value) {
    if (!resize_input_queue_if_need(wl_window, 1)) {
        return;
    }

    wl_window->input_queue[wl_window->input_queue_size++] = (CpdInputEvent) {
        .type = CpdInputEventType_MouseScroll,
        .modifiers = g_keyboard.modifiers,
        .time = get_clock_usec(),
        .data.mouse_scroll.vertical_scroll = axis == WL_POINTER_AXIS_VERTICAL_SCROLL ? value : 0,
        .data.mouse_scroll.horizontal_scroll = axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL ? value : 0
    };
}

static void add_current_mouse_event(CpdWaylandWindow* wl_window) {
    if (!resize_input_queue_if_need(wl_window, 1)) {
        return;
    }

    g_current_pointer_event.modifiers = g_keyboard.modifiers;
    g_current_pointer_event.time = get_clock_usec();

    wl_window->input_queue[wl_window->input_queue_size++] = g_current_pointer_event;

    if (g_current_pointer_event.type == CpdInputEventType_MouseMove) {
        wl_window->mouse_x = g_current_pointer_event.data.mouse_move.x_pos;
        wl_window->mouse_y = g_current_pointer_event.data.mouse_move.y_pos;
    }

    g_current_pointer_event.type = CpdInputEventType_None;
}

static void shift_input_event_right(CpdWaylandWindow* wl_window, uint32_t start_index, uint32_t shift_size) {
    for (uint32_t i = wl_window->input_queue_size; i >= start_index; i--) {
        wl_window->input_queue[i + shift_size] = wl_window->input_queue[i];
    }
}

void insert_repeating_key_events(CpdWaylandWindow* wl_window) {
    if (g_keyboard.repeat_rate == 0 || g_keyboard.pressed_buttons == 0 || wl_window->input_mode != CpdInputMode_Text) {
        return;
    }

    uint64_t moment = get_clock_usec();
    uint64_t from_time = wl_window->last_repeating_key_events_insert_time;
    uint32_t new_events = 0;

    for (CpdPressedButton* current = g_keyboard.pressed_buttons; current != 0; current = current->next) {
        uint64_t repeat_start = current->time + (uint64_t)g_keyboard.repeat_delay * 1000;
        uint64_t rate_fraction = 1000000 / g_keyboard.repeat_rate;

        if (from_time > repeat_start) {
            uint64_t diff = from_time - repeat_start;
            uint64_t remainder = diff % rate_fraction;

            if (remainder != 0) {
                diff += rate_fraction - remainder;
            }

            repeat_start += diff;
        }

        if (repeat_start > moment) {
            continue;
        }

        uint64_t time_diff = moment - repeat_start;

        new_events += (uint32_t)((float)(time_diff * g_keyboard.repeat_rate) / 1000000);
    }

    if (new_events == 0 || !resize_input_queue_if_need(wl_window, new_events)) {
        return;
    }

    wl_window->last_repeating_key_events_insert_time = moment;

    for (CpdPressedButton* current = g_keyboard.pressed_buttons; current != 0; current = current->next) {
        uint64_t repeat_start = current->time + (uint64_t)g_keyboard.repeat_delay * 1000;
        uint64_t rate_fraction = 1000000 / g_keyboard.repeat_rate;

        if (from_time > repeat_start) {
            uint64_t diff = from_time - repeat_start;
            uint64_t remainder = diff % rate_fraction;

            if (remainder != 0) {
                diff += rate_fraction - remainder;
            }

            repeat_start += diff;
        }

        uint64_t character = 0;
        uint32_t length = xkb_state_key_get_utf8(g_keyboard.state, current->xkb_key_code, (char*)&character, sizeof(uint32_t));

        if (length == 0) {
            continue;
        }

        for (uint64_t time = repeat_start; time <= moment; time += rate_fraction) {
            uint32_t insert_index = wl_window->input_queue_size;

            for (uint32_t event = 0; event < wl_window->input_queue_size; event++) {
                if (wl_window->input_queue[event].time > time) {
                    shift_input_event_right(wl_window, event, 1);
                    insert_index = event;
                    break;
                }
            }

            wl_window->input_queue_size++;
            wl_window->input_queue[insert_index] = (CpdInputEvent) {
                .type = CpdInputEventType_CharInput,
                .modifiers = g_keyboard.modifiers,
                .time = time,
                .data.char_input.character = character,
                .data.char_input.length = length
            };
        }
    }
}


// ============
// |   MISC   |
// ============


static CpdKeyCode map_key_code(uint32_t keyCode) {
    if (keyCode >= KEY_1 && keyCode <= KEY_9) {
        return keyCode + (CpdKeyCode_1 - KEY_1);
    }

    if (keyCode >= KEY_F1 && keyCode <= KEY_F10) {
        return keyCode - (KEY_F1 - CpdKeyCode_F1);
    }

    if (keyCode >= KEY_KP7 && keyCode <= KEY_KP9) {
        return keyCode + (CpdKeyCode_Numpad7 - KEY_KP7);
    }

    if (keyCode >= KEY_KP4 && keyCode <= KEY_KP6) {
        return keyCode + (CpdKeyCode_Numpad4 - KEY_KP4);
    }

    if (keyCode >= KEY_KP1 && keyCode <= KEY_KP3) {
        return keyCode + (CpdKeyCode_Numpad1 - KEY_KP1);
    }

    if (keyCode >= KEY_F13 && keyCode <= KEY_F24) {
        return keyCode - (KEY_F13 - CpdKeyCode_F13);
    }

    switch (keyCode) {
    case KEY_ESC:       return CpdKeyCode_Escape;
    case KEY_0:         return CpdKeyCode_0;
    case KEY_MINUS:     case KEY_KPMINUS: return CpdKeyCode_Minus;
    case KEY_EQUAL:     case KEY_KPEQUAL: return CpdKeyCode_Equal;
    case KEY_BACKSPACE: return CpdKeyCode_Backspace;
    case KEY_TAB:       return CpdKeyCode_Tab;

    case KEY_Q: return CpdKeyCode_Q;
    case KEY_W: return CpdKeyCode_W;
    case KEY_E: return CpdKeyCode_E;
    case KEY_R: return CpdKeyCode_R;
    case KEY_T: return CpdKeyCode_T;
    case KEY_Y: return CpdKeyCode_Y;
    case KEY_U: return CpdKeyCode_U;
    case KEY_I: return CpdKeyCode_I;
    case KEY_O: return CpdKeyCode_O;
    case KEY_P: return CpdKeyCode_P;

    case KEY_LEFTBRACE:  return CpdKeyCode_LeftSquareBracket;
    case KEY_RIGHTBRACE: return CpdKeyCode_RightSquareBracket;

    case KEY_ENTER:    case KEY_KPENTER:   return CpdKeyCode_Enter;
    case KEY_LEFTCTRL: case KEY_RIGHTCTRL: return CpdKeyCode_Control;

    case KEY_A: return CpdKeyCode_A;
    case KEY_S: return CpdKeyCode_S;
    case KEY_D: return CpdKeyCode_D;
    case KEY_F: return CpdKeyCode_F;
    case KEY_G: return CpdKeyCode_G;
    case KEY_H: return CpdKeyCode_H;
    case KEY_J: return CpdKeyCode_J;
    case KEY_K: return CpdKeyCode_K;
    case KEY_L: return CpdKeyCode_L;

    case KEY_SEMICOLON:  return CpdKeyCode_Semicolon;
    case KEY_APOSTROPHE: return CpdKeyCode_Quote;
    case KEY_GRAVE:      return CpdKeyCode_Backtick;
    case KEY_LEFTSHIFT:  case KEY_RIGHTSHIFT: return CpdKeyCode_Shift;
    case KEY_BACKSLASH:  case KEY_102ND:      return CpdKeyCode_Backslash;

    case KEY_Z: return CpdKeyCode_Z;
    case KEY_X: return CpdKeyCode_X;
    case KEY_C: return CpdKeyCode_C;
    case KEY_V: return CpdKeyCode_V;
    case KEY_B: return CpdKeyCode_B;
    case KEY_N: return CpdKeyCode_N;
    case KEY_M: return CpdKeyCode_M;

    case KEY_COMMA:      return CpdKeyCode_Comma;
    case KEY_DOT:        case KEY_KPDOT:    return CpdKeyCode_Dot;
    case KEY_SLASH:      case KEY_KPSLASH:  return CpdKeyCode_Slash;
    case KEY_KPASTERISK: return CpdKeyCode_Multiply;
    case KEY_LEFTALT:    case KEY_RIGHTALT: return CpdKeyCode_Alt;
    case KEY_SPACE:      return CpdKeyCode_Spacebar;

    case KEY_KPPLUS: return CpdKeyCode_Plus;
    case KEY_KP0:    return CpdKeyCode_Numpad0;
    case KEY_F11:    return CpdKeyCode_F11;
    case KEY_F12:    return CpdKeyCode_F12;

    case KEY_HOME:     return CpdKeyCode_Home;
    case KEY_PAGEUP:   return CpdKeyCode_PageUp;
    case KEY_END:      return CpdKeyCode_End;
    case KEY_PAGEDOWN: return CpdKeyCode_PageDown;
    case KEY_INSERT:   return CpdKeyCode_Insert;
    case KEY_DELETE:   return CpdKeyCode_Delete;

    case KEY_UP:    return CpdKeyCode_UpArrow;
    case KEY_LEFT:  return CpdKeyCode_LeftArrow;
    case KEY_RIGHT: return CpdKeyCode_RightArrow;
    case KEY_DOWN:  return CpdKeyCode_DownArrow;

    default: return CpdKeyCode_Invalid;
    }
}

static CpdPressedButton* find_entry_by_key_code(CpdKeyCode keyCode) {
    CpdPressedButton* entry = g_keyboard.pressed_buttons;
    while (entry != 0 && entry->key_code != keyCode) {
        entry = entry->next;
    }

    return entry;
}

static CpdPressedButton* find_entry_before(CpdPressedButton* entry) {
    CpdPressedButton* current = g_keyboard.pressed_buttons;

    if (current == 0 || current == entry) {
        return 0;
    }

    while (current != 0 && current->next != entry) {
        current = current->next;
    }

    return current;
}
