#pragma once

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Core.h>

extern "C" {
#include <Cpeed/backend.h>
#include "uwpmain.h"
}

class AppView : public winrt::implements<AppView, winrt::Windows::ApplicationModel::Core::IFrameworkView>
{
public:
    // IFrameworkView methods.
    void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& applicationView);
    void SetWindow(winrt::Windows::UI::Core::CoreWindow const& window);
    void Load(winrt::hstring const& entryPoint);
    void Run();
    void Uninitialize();

protected:
    // Application lifecycle event handlers.
    void OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args);

    // Window event handlers.
    void OnVisibilityChanged(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::VisibilityChangedEventArgs const& args);
    void OnWindowClosed(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::CoreWindowEventArgs const& args);

    void OnSizeChanged(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::WindowSizeChangedEventArgs const& args);

    void OnGamepadConnect(winrt::Windows::Foundation::IUnknown const&, winrt::Windows::Gaming::Input::Gamepad const& gamepad);
    void OnGamepadDisconnect(winrt::Windows::Foundation::IUnknown const&, winrt::Windows::Gaming::Input::Gamepad const& gamepad);

    void OnKey(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::KeyEventArgs const& args);
    void OnAcceleratorKey(winrt::Windows::UI::Core::CoreDispatcher const& sender, winrt::Windows::UI::Core::AcceleratorKeyEventArgs const& args);
    void OnCharacterReceived(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::CharacterReceivedEventArgs const& args);

    void OnPointerButton(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::PointerEventArgs const& args);
    void OnPointerMoved(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::PointerEventArgs const& args);
    void OnPointerWheel(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::PointerEventArgs const& args);

private:
    CpdUWPWindow* window;
    CpdBackendImplementation impl;

    // Event registration tokens.
    winrt::event_token m_windowClosedEventToken;
    winrt::event_token m_visibilityChangedEventToken;

    winrt::event_token gamepad_added;
    winrt::event_token gamepad_removed;

    winrt::event_token resize_token;

    winrt::event_token key_up_token;
    winrt::event_token key_down_token;
    winrt::event_token accelerator_token;
    winrt::event_token character_token;

    winrt::event_token pointer_key_up_token;
    winrt::event_token pointer_key_down_token;
    winrt::event_token pointer_move_token;
    winrt::event_token pointer_wheel_token;
};

class AppViewSource : public winrt::implements<AppViewSource, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
{
public:
    // IFrameworkViewSource method.
    winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();
};
