#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum CpdGamepadButtonType {
    CpdGamepadButtonType_Invalid,
    CpdGamepadButtonType_X,
    CpdGamepadButtonType_Y,
    CpdGamepadButtonType_B,
    CpdGamepadButtonType_A,
    CpdGamepadButtonType_DPadLeft,
    CpdGamepadButtonType_DPadUp,
    CpdGamepadButtonType_DPadRight,
    CpdGamepadButtonType_DPadDown,
    CpdGamepadButtonType_ShoulderLeft,
    CpdGamepadButtonType_ShoulderRight,
    CpdGamepadButtonType_StickLeft,
    CpdGamepadButtonType_StickRight,
    CpdGamepadButtonType_Menu,
    CpdGamepadButtonType_View
} CpdGamepadButtonType;

typedef enum CpdGamepadStick {
    CpdGamepadStick_Left,
    CpdGamepadStick_Right
} CpdGamepadStick;

typedef enum CpdGamepadTrigger {
    CpdGamepadTrigger_Left,
    CpdGamepadTrigger_Right
} CpdGamepadTrigger;

typedef enum CpdGamepadConnectStatus {
    CpdGamepadConnectStatus_Disconnected,
    CpdGamepadConnectStatus_Connected
} CpdGamepadConnectStatus;

typedef struct CpdGamepadStickPosition {
    float x;
    float y;
} CpdGamepadStickPosition;

typedef struct CpdGamepadTriggersPosition {
    float left;
    float right;
} CpdGamepadTriggersPosition;

typedef struct CpdGamepadButtonPressInputEventData {
    CpdGamepadButtonType button;
    uint16_t gamepad_index;
    bool pressed;
} CpdGamepadButtonPressInputEventData;

typedef struct CpdGamepadStickInputEventData {
    CpdGamepadStick stick;
    uint16_t gamepad_index;
} CpdGamepadStickInputEventData;

typedef struct CpdGamepadTriggerInputEventData {
    CpdGamepadTrigger trigger;
    uint16_t gamepad_index;
} CpdGamepadTriggerInputEventData;

typedef struct CpdGamepadConnectInputEventData {
    CpdGamepadConnectStatus status;
    uint16_t gamepad_index;
} CpdGamepadConnectInputEventData;
