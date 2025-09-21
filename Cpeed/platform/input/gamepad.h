#pragma once

#include "../../input.h"
#include "../window.h"

extern uint16_t get_gamepad_count(CpdWindow window);
extern CpdGamepadStickPosition get_gamepad_stick_position(CpdWindow window, uint16_t gamepad_index, CpdGamepadStick stick);
extern CpdGamepadTriggersPosition get_gamepad_triggers_position(CpdWindow window, uint16_t gamepad_index);
