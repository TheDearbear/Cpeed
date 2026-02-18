#pragma once

#include <Cpeed/input/gamepad.h>
#include <Cpeed/input/keyboard.h>
#include <Cpeed/input/mouse.h>

bool add_button_press_to_queue(struct CpdUWPWindow* uwp_window, CpdKeyCode keyCode, bool pressed);
bool add_char_input_to_queue(struct CpdUWPWindow* uwp_window, uint32_t character, uint32_t length);
bool add_mouse_button_press_to_queue(struct CpdUWPWindow* uwp_window, CpdMouseButtonType button, bool pressed);
bool add_mouse_move_to_queue(struct CpdUWPWindow* uwp_window, int16_t x_pos, int16_t y_pos);
bool add_mouse_scroll_to_queue(struct CpdUWPWindow* uwp_window, int32_t vertical, int32_t horizontal);
bool add_gamepad_connect_to_queue(struct CpdUWPWindow* uwp_window, CpdGamepadConnectStatus status, uint16_t index);
bool add_gamepad_stick_to_queue(struct CpdUWPWindow* uwp_window, CpdGamepadStick stick, uint16_t index);
bool add_gamepad_trigger_to_queue(struct CpdUWPWindow* uwp_window, CpdGamepadTrigger trigger, uint16_t index);
bool add_gamepad_button_press_to_queue(struct CpdUWPWindow* uwp_window, CpdGamepadButtonType button, uint16_t index, bool pressed);
