#pragma once

#define CINTERFACE
#define COBJMACROS

#include <stdbool.h>
#include <GameInput.h>

extern IGameInput* g_game_input;
extern struct CpdGamepad* g_gamepads;

typedef struct CpdGamepad {
    struct CpdGamepad* next;
    IGameInputDevice* device;
    GameInputGamepadState last_used_state;
} CpdGamepad;
