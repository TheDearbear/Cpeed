#include <unknwn.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.UI.Input.h>

#include "AppView.h"

extern "C" {
#include "../platform/init.h"
}

using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Gaming::Input;
using namespace winrt::Windows::UI::Core;

static inline void check_gamepad_button(CpdUWPWindow* uwp_window, GamepadReading* state, CpdInputDevice* device, uint16_t index, GamepadButtons state_button, CpdGamepadButtonType cpeed_button) {
    uint32_t state_button_u32 = (uint32_t)state_button;

    if (((uint32_t)state->Buttons & state_button_u32) != (device->state.Buttons & state_button_u32)) {
        add_gamepad_button_press_to_queue(uwp_window, cpeed_button, index, ((uint32_t)state->Buttons & state_button_u32) != 0);
    }
}

extern "C" {
    void poll_events(CpdUWPWindow* uwp_window) {
        if (uwp_window->visible)
        {
            CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        }
        else
        {
            CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }

    void poll_gamepads(CpdUWPWindow* uwp_window) {
        CpdInputDevice* current = uwp_window->devices;
        uint32_t index = 0;

        for (auto const& gamepad : Gamepad::Gamepads()) {
            GamepadReading state{ gamepad.GetCurrentReading() };

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

            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::Menu, CpdGamepadButtonType_Menu);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::View, CpdGamepadButtonType_View);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::A, CpdGamepadButtonType_A);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::B, CpdGamepadButtonType_B);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::X, CpdGamepadButtonType_X);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::Y, CpdGamepadButtonType_Y);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::DPadUp, CpdGamepadButtonType_DPadUp);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::DPadDown, CpdGamepadButtonType_DPadDown);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::DPadLeft, CpdGamepadButtonType_DPadLeft);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::DPadRight, CpdGamepadButtonType_DPadRight);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::LeftShoulder, CpdGamepadButtonType_ShoulderLeft);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::RightShoulder, CpdGamepadButtonType_ShoulderRight);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::LeftThumbstick, CpdGamepadButtonType_StickLeft);
            check_gamepad_button(uwp_window, &state, current, index, GamepadButtons::RightThumbstick, CpdGamepadButtonType_StickRight);

            memcpy_s(&current->state, sizeof(__x_ABI_CWindows_CGaming_CInput_CGamepadReading), &state, sizeof(GamepadReading));

            current = current->next;
            index++;
        }
    }

    int real_main() {
        if (!initialize_platform()) {
            return 0;
        }

        winrt::init_apartment();
        CoreApplication::Run(winrt::make<AppViewSource>());

        shutdown_platform();

        return 0;
    }
}
