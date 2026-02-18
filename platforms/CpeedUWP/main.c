#define COBJMACROS

#include <roapi.h>
#include <string.h>
#include <winstring.h>

#include "uwpmain.h"
#include "uwpqueue.h"

#include <Windows.System.Profile.h>
#include <Windows.UI.Core.h>

EXTERN_C const IID IID___x_Windows_CSystem_CProfile_CIAnalyticsInfoStatics = { 0x1D5EE066, 0x188D, 0x5BA9, 0x43, 0x87, 0xAC, 0xAE, 0xB0, 0xE7, 0xE3, 0x05 };
EXTERN_C const IID IID___x_Windows_CGaming_CInput_CIGamepadStatics = { 0x8BBCE529, 0xD49C, 0x39E9, 0x95, 0x60, 0xE4, 0x7D, 0xDE, 0x96, 0xB7, 0xC8 };
EXTERN_C const IID IID___x_Windows_CUI_CCore_CICoreWindowStatic;

static inline void check_gamepad_button(CpdUWPWindow* uwp_window, __x_ABI_CWindows_CGaming_CInput_CGamepadReading* state, CpdInputDevice* device, uint16_t index, __x_ABI_CWindows_CGaming_CInput_CGamepadButtons state_button, CpdGamepadButtonType cpeed_button) {
    uint32_t state_button_u32 = (uint32_t)state_button;

    if (((uint32_t)state->Buttons & state_button_u32) != (device->state.Buttons & state_button_u32)) {
        add_gamepad_button_press_to_queue(uwp_window, cpeed_button, index, ((uint32_t)state->Buttons & state_button_u32) != 0);
    }
}

void poll_events(CpdUWPWindow* uwp_window) {
    HSTRING str;
    HRESULT result = WindowsCreateString(RuntimeClass_Windows_UI_Core_CoreWindow, (UINT32)wcslen(RuntimeClass_Windows_UI_Core_CoreWindow), &str);

    if (FAILED(result) || str == 0) {
        return;
    }

    __x_ABI_CWindows_CUI_CCore_CICoreWindowStatic* window_static = 0;
    result = RoGetActivationFactory(str, &IID___x_Windows_CUI_CCore_CICoreWindowStatic, &window_static);
    WindowsDeleteString(str);

    if (FAILED(result)) {
        return;
    }

    __x_ABI_CWindows_CUI_CCore_CICoreWindow* window;
    result = __x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(window_static, &window);
    __x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(window_static);

    if (FAILED(result)) {
        return;
    }

    __x_ABI_CWindows_CUI_CCore_CICoreDispatcher* dispatcher;
    result = __x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Dispatcher(window, &dispatcher);
    __x_ABI_CWindows_CUI_CCore_CICoreWindow_Release(window);

    if (FAILED(result)) {
        return;
    }

    enum __x_ABI_CWindows_CUI_CCore_CCoreProcessEventsOption option;
    if (uwp_window->visible) {
        option = CoreProcessEventsOption_ProcessAllIfPresent;
    }
    else {
        option = CoreProcessEventsOption_ProcessOneAndAllPending;
    }

    __x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(dispatcher, option);
    __x_ABI_CWindows_CUI_CCore_CICoreDispatcher_Release(dispatcher);
}

void poll_gamepads(CpdUWPWindow* uwp_window) {
    CpdInputDevice* current = uwp_window->devices;
    uint32_t index = 0;
    HSTRING str;

    HRESULT result = WindowsCreateString(RuntimeClass_Windows_Gaming_Input_Gamepad, (UINT32)wcslen(RuntimeClass_Windows_Gaming_Input_Gamepad), &str);

    if (FAILED(result) || str == 0) {
        return;
    }

    __x_ABI_CWindows_CGaming_CInput_CIGamepadStatics* gamepad_statics = 0;
    result = RoGetActivationFactory(str, &IID___x_Windows_CGaming_CInput_CIGamepadStatics, &gamepad_statics);
    WindowsDeleteString(str);

    if (FAILED(result)) {
        return;
    }

    __FIVectorView_1_Windows__CGaming__CInput__CGamepad* gamepads;
    result = __x_ABI_CWindows_CGaming_CInput_CIGamepadStatics_get_Gamepads(gamepad_statics, &gamepads);
    __x_ABI_CWindows_CGaming_CInput_CIGamepadStatics_Release(gamepad_statics);

    if (FAILED(result)) {
        return;
    }

    UINT32 size = 0;
    __FIVectorView_1_Windows__CGaming__CInput__CGamepad_get_Size(gamepads, &size);

    for (UINT32 i = 0; i < size; i++) {
        __x_ABI_CWindows_CGaming_CInput_CIGamepad* gamepad;
        __FIVectorView_1_Windows__CGaming__CInput__CGamepad_GetAt(gamepads, i, &gamepad);

        __x_ABI_CWindows_CGaming_CInput_CGamepadReading state;
        __x_ABI_CWindows_CGaming_CInput_CIGamepad_GetCurrentReading(gamepad, &state);
        __x_ABI_CWindows_CGaming_CInput_CIGamepad_Release(gamepad);

        if (state.LeftThumbstickX != current->state.LeftThumbstickX || state.LeftThumbstickY != current->state.LeftThumbstickY) {
            add_gamepad_stick_to_queue(uwp_window, CpdGamepadStick_Left, index);
        }

        if (state.RightThumbstickX != current->state.RightThumbstickX || state.RightThumbstickY != current->state.RightThumbstickY) {
            add_gamepad_stick_to_queue(uwp_window, CpdGamepadStick_Right, index);
        }

        if (state.LeftTrigger != current->state.LeftTrigger) {
            add_gamepad_trigger_to_queue(uwp_window, CpdGamepadTrigger_Left, index);
        }

        if (state.RightTrigger != current->state.RightTrigger) {
            add_gamepad_trigger_to_queue(uwp_window, CpdGamepadTrigger_Right, index);
        }

        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_Menu, CpdGamepadButtonType_Menu);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_View, CpdGamepadButtonType_View);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_A, CpdGamepadButtonType_A);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_B, CpdGamepadButtonType_B);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_X, CpdGamepadButtonType_X);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_Y, CpdGamepadButtonType_Y);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_DPadUp, CpdGamepadButtonType_DPadUp);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_DPadDown, CpdGamepadButtonType_DPadDown);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_DPadLeft, CpdGamepadButtonType_DPadLeft);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_DPadRight, CpdGamepadButtonType_DPadRight);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_LeftShoulder, CpdGamepadButtonType_ShoulderLeft);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_RightShoulder, CpdGamepadButtonType_ShoulderRight);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_LeftThumbstick, CpdGamepadButtonType_StickLeft);
        check_gamepad_button(uwp_window, &state, current, index, GamepadButtons_RightThumbstick, CpdGamepadButtonType_StickRight);

        current->state = state;

        current = current->next;
        index++;
    }

    __FIVectorView_1_Windows__CGaming__CInput__CGamepad_Release(gamepads);
}

bool get_lowest_frame_layer(void* context, struct CpdFrameLayer* frame_layer) {
    struct CpdFrameLayer** output = (struct CpdFrameLayer**)context;

    *output = frame_layer;

    return true;
}

bool windowed_mode_supported() {
    HSTRING str;
    HRESULT result = WindowsCreateString(RuntimeClass_Windows_System_Profile_AnalyticsInfo, (UINT32)wcslen(RuntimeClass_Windows_System_Profile_AnalyticsInfo), &str);

    if (FAILED(result) || str == 0) {
        return false;
    }

    __x_ABI_CWindows_CSystem_CProfile_CIAnalyticsInfoStatics* info_statics = 0;
    result = RoGetActivationFactory(str, &IID___x_Windows_CSystem_CProfile_CIAnalyticsInfoStatics, &info_statics);
    WindowsDeleteString(str);

    if (FAILED(result)) {
        return false;
    }

    __x_ABI_CWindows_CSystem_CProfile_CIAnalyticsVersionInfo* version_info;
    result = __x_ABI_CWindows_CSystem_CProfile_CIAnalyticsInfoStatics_get_VersionInfo(info_statics, &version_info);
    __x_ABI_CWindows_CSystem_CProfile_CIAnalyticsInfoStatics_Release(info_statics);

    if (FAILED(result)) {
        return false;
    }

    result = __x_ABI_CWindows_CSystem_CProfile_CIAnalyticsVersionInfo_get_DeviceFamily(version_info, &str);
    __x_ABI_CWindows_CSystem_CProfile_CIAnalyticsVersionInfo_Release(version_info);

    if (FAILED(result)) {
        return false;
    }

    UINT32 length = 0;
    const wchar_t* device_family = WindowsGetStringRawBuffer(str, &length);
    static const wchar_t desktop_family[] = L"Windows.Desktop";

    bool is_equal = (sizeof(desktop_family) / sizeof(wchar_t)) - 1 == length && wcscmp(device_family, desktop_family) == 0;

    WindowsDeleteString(str);

    return is_equal;
}
