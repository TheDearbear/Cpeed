// Enable winrt::get_unknown in old Windows SDKs
#define WINRT_WINDOWS_ABI

#include <unknwn.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "AppView.h"

extern "C" {
#ifdef CPD_IMGUI_AVAILABLE
#include "../common/imgui/imgui_impl_cpeed.h"
#endif

#include "../common/init.h"
#include "../platform/input/queue.h"
}

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Gaming::Input;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::System;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::ViewManagement;

const USHORT SCANCODE_LSHIFT = 42;
const USHORT SCANCODE_RSHIFT = 54;

void AppView::OnGamepadConnect(winrt::Windows::Foundation::IUnknown const&, Gamepad const& gamepad) {
    CpdInputDevice* entry = (CpdInputDevice*)malloc(sizeof(CpdInputDevice));
    if (entry == 0) {
        free(entry);
        return;
    }

    GamepadReading reading{ gamepad.GetCurrentReading() };
    memcpy_s(&entry->state, sizeof(__x_ABI_CWindows_CGaming_CInput_CGamepadReading), &reading, sizeof(GamepadReading));

    uint16_t index = 0;
    entry->next = 0;

    if (window->devices == 0) {
        window->devices = entry;
    }
    else {
        CpdInputDevice* current = window->devices;
        while (current->next != 0) {
            current = current->next;
            index++;
        }

        current->next = entry;
        index++;
    }

    add_gamepad_connect_to_queue(window, CpdGamepadConnectStatus_Connected, index);
}

void AppView::OnGamepadDisconnect(winrt::Windows::Foundation::IUnknown const&, Gamepad const& gamepad) {
    CpdInputDevice* previous = 0;
    CpdInputDevice* current = window->devices;
    uint16_t index = 0;

    for (Gamepad const& current_gamepad : Gamepad::Gamepads()) {
        if (winrt::get_abi(current_gamepad) == winrt::get_abi(gamepad)) {
            break;
        }

        previous = current;
        current = current->next;
        index++;
    }

    if (current == 0) {
        return;
    }

    add_gamepad_connect_to_queue(window, CpdGamepadConnectStatus_Disconnected, index);

    if (previous != 0) {
        previous->next = current->next;
    }
    else {
        window->devices = current->next;
    }

    free(current);
}

void AppView::Initialize(CoreApplicationView const& applicationView)
{
    if (!windowed_mode_supported()) {
        ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::FullScreen);
    }

    applicationView.Activated({ this, &AppView::OnViewActivated });

    gamepad_added = Gamepad::GamepadAdded({ this, &AppView::OnGamepadConnect });
    gamepad_removed = Gamepad::GamepadRemoved({ this, &AppView::OnGamepadDisconnect });

    for (auto const& gamepad : Gamepad::Gamepads()) {
        OnGamepadConnect(nullptr, gamepad);
    }

    window = (CpdUWPWindow*)create_window(&g_window_create_info);

#ifdef CPD_IMGUI_AVAILABLE
    if (!cImGui_ImplCpeed_Init(window)) {
        __debugbreak();
    }
#endif

    get_backend_implementation(CpdPlatformBackendFlags_DirectX, &impl);

    impl.initialize_backend();

    init_engine(window);
}

// Called when the CoreWindow object is created (or re-created).
void AppView::SetWindow(CoreWindow const& window)
{
    if (this->window->core_window != 0) {
        impl.shutdown_window((CpdWindow)this->window);
    }

    this->window->core_window = (void*)winrt::get_unknown(window);
    auto bounds = window.Bounds();

    double screen_factor = DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();

    this->window->size.width = (unsigned short)(bounds.Width * screen_factor);
    this->window->size.height = (unsigned short)(bounds.Height * screen_factor);
    this->window->resized = true;

    CpdBackendInfo backend_info = { (CpdWindow)this->window, { 0.2f, 0.5f, 0.5f } };

    this->window->backend = impl.initialize_window(&backend_info);
    if (this->window->backend == 0) {
        __debugbreak();
    }

    // Register for notification that the app window is being closed.
    m_windowClosedEventToken = window.Closed({ this, &AppView::OnWindowClosed });

    // Register for notifications that the app window is losing focus.
    m_visibilityChangedEventToken = window.VisibilityChanged({ this, &AppView::OnVisibilityChanged });

    resize_token = window.SizeChanged({ this, &AppView::OnSizeChanged });

    key_up_token = window.KeyUp({ this, &AppView::OnKey });
    key_down_token = window.KeyDown({ this, &AppView::OnKey });
    accelerator_token = window.Dispatcher().AcceleratorKeyActivated({ this, &AppView::OnAcceleratorKey });
    character_token = window.CharacterReceived({ this, &AppView::OnCharacterReceived });

    pointer_key_up_token = window.PointerReleased({ this, &AppView::OnPointerButton });
    pointer_key_down_token = window.PointerPressed({ this, &AppView::OnPointerButton });
    pointer_move_token = window.PointerMoved({ this, &AppView::OnPointerMoved });
    pointer_wheel_token = window.PointerWheelChanged({ this, &AppView::OnPointerWheel });
}

// The Load method can be used to initialize scene resources or to load a
// previously saved app state.
void AppView::Load(winrt::hstring const& entryPoint)
{
}

// This method is called after the window becomes active. It oversees the
// update, draw, and present loop, and it also oversees window message processing.
void AppView::Run()
{
    CpdFrame* frame = impl.get_frame(window->backend);

    while (!poll_window((CpdWindow)window))
    {
        bool resized = window_resized((CpdWindow)window);
        if (resized) {
            CpdSize new_size = window_size((CpdWindow)window);

            if (!impl.resize(window->backend, new_size)) {
                __debugbreak();
            }
        }

        uint32_t input_event_count = 0;
        const CpdInputEvent* input_events = 0;
        if (get_window_input_events((CpdWindow)window, &input_events, &input_event_count)) {
            for (uint32_t i = 0; i < input_event_count; i++) {
                CpdFrameLayer* frame_layer = 0;
                loop_frame_layers((CpdWindow)window, get_lowest_frame_layer, &frame_layer);

                while (frame_layer != 0) {
                    if (frame_layer->functions.input != 0 && !frame_layer->functions.input(window, frame, &input_events[i])) {
                        break;
                    }

                    frame_layer = frame_layer->higher;
                }
            }
        }

        if (!impl.should_frame(window->backend, (CpdWindow)window)) {
            continue;
        }

        if (!impl.pre_frame(window->backend)) {
            __debugbreak();
        }

        impl.frame(window->backend);

        impl.post_frame(window->backend);
    }
}

void AppView::Uninitialize()
{
    auto const& window = CoreWindow::GetForCurrentThread();

    Gamepad::GamepadAdded(gamepad_added);
    Gamepad::GamepadRemoved(gamepad_removed);

    CpdInputDevice* current = this->window->devices;
    while (current != 0) {
        CpdInputDevice* next = current->next;

        free(current);

        current = next;
    }

    window.Closed(m_windowClosedEventToken);
    window.VisibilityChanged(m_visibilityChangedEventToken);

    window.KeyUp(key_up_token);
    window.KeyDown(key_down_token);
    window.Dispatcher().AcceleratorKeyActivated(accelerator_token);
    window.CharacterReceived(character_token);

    window.PointerReleased(pointer_key_up_token);
    window.PointerPressed(pointer_key_down_token);
    window.PointerMoved(pointer_move_token);
    window.PointerWheelChanged(pointer_wheel_token);

    shutdown_engine();

#ifdef CPD_IMGUI_AVAILABLE
    cImGui_ImplCpeed_Shutdown();
#endif
}

void AppView::OnViewActivated(CoreApplicationView const& sender, IActivatedEventArgs const& args)
{
    sender.CoreWindow().Activate();
}

// Window event handlers

void AppView::OnVisibilityChanged(CoreWindow const& sender, VisibilityChangedEventArgs const& args)
{
    window->visible = args.Visible();
}

void AppView::OnWindowClosed(CoreWindow const& sender, CoreWindowEventArgs const& args)
{
    window->should_close = true;
}

void AppView::OnSizeChanged(CoreWindow const& sender, WindowSizeChangedEventArgs const& args) {
    double screen_factor = DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();

    window->size.width = (uint16_t)(args.Size().Width * screen_factor);
    window->size.height = (uint16_t)(args.Size().Height * screen_factor);
    window->resized = true;
}

static CpdKeyCode virtual_key_to_key_code(KeyEventArgs const& args) {
    VirtualKey vk = args.VirtualKey();
    int32_t vk_int = (int32_t)args.VirtualKey();

    if (vk >= VirtualKey::F1 && vk <= VirtualKey::F24) {
        return (CpdKeyCode)(vk_int - ((int32_t)VirtualKey::F1 - CpdKeyCode_F1));
    }

    if (vk_int >= 'A' && vk_int <= 'Z') {
        return (CpdKeyCode)(vk_int - ('A' - CpdKeyCode_A));
    }

    if (vk_int >= '0' && vk_int <= '9') {
        return (CpdKeyCode)(vk_int - ('0' - CpdKeyCode_0));
    }

    if (vk >= VirtualKey::NumberPad0 && vk <= VirtualKey::NumberPad9) {
        return (CpdKeyCode)(vk_int - ((int32_t)VirtualKey::NumberPad0 - CpdKeyCode_Numpad0));
    }

    if (vk == VirtualKey::Shift) {
        return args.KeyStatus().ScanCode != SCANCODE_RSHIFT ? CpdKeyCode_LeftShift : CpdKeyCode_RightShift;
    }

    switch (vk) {
        case VirtualKey::Back:         return CpdKeyCode_Backspace;
        case VirtualKey::Tab:          return CpdKeyCode_Tab;
        case VirtualKey::Enter:        return args.KeyStatus().IsExtendedKey ? CpdKeyCode_NumpadEnter : CpdKeyCode_Enter;
        case VirtualKey::Control:      return args.KeyStatus().IsExtendedKey ? CpdKeyCode_RightControl : CpdKeyCode_LeftControl;
        case VirtualKey::Escape:       return CpdKeyCode_Escape;
        case VirtualKey::Space:        return CpdKeyCode_Spacebar;
        case VirtualKey::PageUp:       return CpdKeyCode_PageUp;
        case VirtualKey::PageDown:     return CpdKeyCode_PageDown;
        case VirtualKey::End:          return CpdKeyCode_End;
        case VirtualKey::Home:         return CpdKeyCode_Home;
        case VirtualKey::Left:         return CpdKeyCode_LeftArrow;
        case VirtualKey::Right:        return CpdKeyCode_RightArrow;
        case VirtualKey::Up:           return CpdKeyCode_UpArrow;
        case VirtualKey::Down:         return CpdKeyCode_DownArrow;
        case VirtualKey::Insert:       return CpdKeyCode_Insert;
        case VirtualKey::Delete:       return CpdKeyCode_Delete;
        case VirtualKey::Multiply:     return CpdKeyCode_Multiply;
        case VirtualKey::Subtract:     case (VirtualKey)VK_OEM_MINUS:  return CpdKeyCode_Minus;
        case VirtualKey::Add:          return CpdKeyCode_Plus;
        case (VirtualKey)VK_OEM_PLUS:  return CpdKeyCode_Equal;
        case VirtualKey::Divide:       case (VirtualKey)VK_OEM_2:      return CpdKeyCode_Slash;
        case (VirtualKey)VK_OEM_5:     case (VirtualKey)VK_OEM_102:    return CpdKeyCode_Backslash;
        case VirtualKey::Decimal:      case (VirtualKey)VK_OEM_PERIOD: return CpdKeyCode_Dot;
        case (VirtualKey)VK_OEM_COMMA: return CpdKeyCode_Comma;
        case (VirtualKey)VK_OEM_1:     return CpdKeyCode_Semicolon;
        case (VirtualKey)VK_OEM_4:     return CpdKeyCode_LeftSquareBracket;
        case (VirtualKey)VK_OEM_6:     return CpdKeyCode_RightSquareBracket;
        case (VirtualKey)VK_OEM_7:     return CpdKeyCode_Quote;
        case (VirtualKey)VK_OEM_3:     return CpdKeyCode_Backtick;
        default:                       return CpdKeyCode_Invalid;
    }
}

static void set_key_modifier(CpdUWPWindow* uwp_window, CpdInputModifierKeyFlags modifier, bool pressed) {
    if (pressed) {
        uwp_window->current_key_modifiers = (CpdInputModifierKeyFlags)(uwp_window->current_key_modifiers | modifier);
    }
    else {
        uwp_window->current_key_modifiers = (CpdInputModifierKeyFlags)(uwp_window->current_key_modifiers & (~modifier));
    }
}

void AppView::OnKey(CoreWindow const& sender, KeyEventArgs const& args) {
    if (args.KeyStatus().WasKeyDown != args.KeyStatus().IsKeyReleased) {
        return;
    }

    if (args.VirtualKey() == VirtualKey::GamepadB) {
        args.Handled(true);
        return;
    }

    bool pressed = !args.KeyStatus().IsKeyReleased;
    CpdKeyCode keyCode = virtual_key_to_key_code(args);
    
    switch (args.VirtualKey()) {
        case VirtualKey::Shift:
            if (!pressed) {
                bool left_pressed = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::LeftShift) == CoreVirtualKeyStates::Down;
                bool right_pressed = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::RightShift) == CoreVirtualKeyStates::Down;

                pressed = left_pressed || right_pressed;
            }

            set_key_modifier(window, CpdInputModifierKey_Shift, pressed);
            break;

        case VirtualKey::Control:
            if (!pressed) {
                bool left_pressed = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::LeftControl) == CoreVirtualKeyStates::Down;
                bool right_pressed = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::RightControl) == CoreVirtualKeyStates::Down;

                pressed = left_pressed || right_pressed;
            }

            set_key_modifier(window, CpdInputModifierKey_Control, pressed);
            break;
    }

    if (keyCode != CpdKeyCode_Invalid) {
        args.Handled(add_button_press_to_queue(window, keyCode, pressed));
    }
}

void AppView::OnAcceleratorKey(CoreDispatcher const& sender, AcceleratorKeyEventArgs const& args) {
    if (args.VirtualKey() != VirtualKey::Menu || args.KeyStatus().WasKeyDown != args.KeyStatus().IsKeyReleased) {
        return;
    }

    bool pressed = !args.KeyStatus().IsKeyReleased;
    CpdKeyCode keyCode = args.KeyStatus().IsExtendedKey ? CpdKeyCode_RightAlt : CpdKeyCode_LeftAlt;

    if (!pressed) {
        bool left_pressed = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::LeftMenu) == CoreVirtualKeyStates::Down;
        bool right_pressed = CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::RightMenu) == CoreVirtualKeyStates::Down;

        pressed = left_pressed || right_pressed;
    }

    set_key_modifier(window, CpdInputModifierKey_Alt, pressed);

    args.Handled(add_button_press_to_queue(window, keyCode, pressed));
}

void AppView::OnCharacterReceived(CoreWindow const& sender, CharacterReceivedEventArgs const& args) {
    if (window->input_mode != CpdInputMode_Text) {
        return;
    }

    uint64_t code = args.KeyCode();
    uint32_t character = 0;
    int length = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)&code, 1, (char*)&character, sizeof(uint32_t), 0, 0);

    if (length == 0) {
        return;
    }

    args.Handled(add_char_input_to_queue(window, character, length));
}

void AppView::OnPointerButton(CoreWindow const& sender, PointerEventArgs const& args) {
    PointerPointProperties const& props = args.CurrentPoint().Properties();

    if (props.PointerUpdateKind() == PointerUpdateKind::Other) {
        return;
    }

    CpdMouseButtonType button = CpdMouseButtonType_Left;
    bool pressed = false;

    switch (props.PointerUpdateKind()) {
        case PointerUpdateKind::LeftButtonPressed:
            button = CpdMouseButtonType_Left;
            pressed = true;
            break;

        case PointerUpdateKind::LeftButtonReleased:
            button = CpdMouseButtonType_Left;
            pressed = false;
            break;

        case PointerUpdateKind::MiddleButtonPressed:
            button = CpdMouseButtonType_Middle;
            pressed = true;
            break;

        case PointerUpdateKind::MiddleButtonReleased:
            button = CpdMouseButtonType_Middle;
            pressed = false;
            break;

        case PointerUpdateKind::RightButtonPressed:
            button = CpdMouseButtonType_Right;
            pressed = true;
            break;

        case PointerUpdateKind::RightButtonReleased:
            button = CpdMouseButtonType_Right;
            pressed = false;
            break;

        case PointerUpdateKind::XButton1Pressed:
            button = CpdMouseButtonType_Extra1;
            pressed = true;
            break;

        case PointerUpdateKind::XButton1Released:
            button = CpdMouseButtonType_Extra1;
            pressed = false;
            break;

        case PointerUpdateKind::XButton2Pressed:
            button = CpdMouseButtonType_Extra2;
            pressed = true;
            break;

        case PointerUpdateKind::XButton2Released:
            button = CpdMouseButtonType_Extra2;
            pressed = false;
            break;

        default:
            return;
    }

    args.Handled(add_mouse_button_press_to_queue(window, button, pressed));
}

void AppView::OnPointerMoved(CoreWindow const& sender, PointerEventArgs const& args) {
    double screen_factor = DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();

    int32_t x = (int32_t)(args.CurrentPoint().Position().X * screen_factor);
    int32_t y = (int32_t)(args.CurrentPoint().Position().Y * screen_factor);

    if (!add_mouse_move_to_queue(window, (int16_t)x, (int16_t)y)) {
        return;
    }

    args.Handled(true);
    window->mouse_x = (int32_t)x;
    window->mouse_y = (int32_t)y;
}

void AppView::OnPointerWheel(CoreWindow const& sender, PointerEventArgs const& args) {
    int32_t distance = args.CurrentPoint().Properties().MouseWheelDelta();

    if (args.CurrentPoint().Properties().IsHorizontalMouseWheel()) {
        args.Handled(add_mouse_scroll_to_queue(window, 0, -distance));
    }
    else {
        args.Handled(add_mouse_scroll_to_queue(window, -distance, 0));
    }
}

IFrameworkView AppViewSource::CreateView() {
    return winrt::make<AppView>();
}
