#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum CpdMouseButtonType {
    CpdMouseButtonType_Left,
    CpdMouseButtonType_Middle,
    CpdMouseButtonType_Right,
    CpdMouseButtonType_Extra1,
    CpdMouseButtonType_Extra2
} CpdMouseButtonType;

typedef struct CpdMouseMoveInputEventData {
    int16_t x_pos;
    int16_t y_pos;
    int16_t x_move;
    int16_t y_move;
} CpdMouseMoveInputEventData;

typedef struct CpdMouseScrollInputEventData {
    int32_t vertical_scroll;
    int32_t horizontal_scroll;
} CpdMouseScrollInputEventData;

typedef struct CpdMouseButtonPressInputEventData {
    CpdMouseButtonType button;
    bool pressed;
} CpdMouseButtonPressInputEventData;
