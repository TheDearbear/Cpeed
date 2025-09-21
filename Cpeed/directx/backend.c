#include <malloc.h>
#include <stdio.h>

#include "../platform/input/gamepad.h"
#include "backend.h"
#include "directx.h"

static bool initialize_backend() {
    return true;
}

static void shutdown_backend() {
    //
}

static void shutdown_window(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    if (renderer->render_target != 0) {
        ID3D11RenderTargetView_Release(renderer->render_target);
    }

    if (renderer->swapchain != 0) {
        IDXGISwapChain1_Release(renderer->swapchain);
    }

    ID3D11DeviceContext_Release(renderer->device_context);
    ID3D11Device_Release(renderer->device);

    free(renderer);
}

static HRESULT create_render_target(CpdDirectXRenderer* renderer) {
    ID3D11Resource* surface = 0;
    HRESULT result = IDXGISwapChain1_GetBuffer(renderer->swapchain, 0, &IID_ID3D11Texture2D, &surface);
    if (result != S_OK) {
        return result;
    }

    D3D11_RENDER_TARGET_VIEW_DESC desc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
        .Texture2D.MipSlice = 0
    };

    result = ID3D11Device_CreateRenderTargetView(renderer->device, surface, &desc, &renderer->render_target);

    ID3D11Resource_Release(surface);
    return result;
}

static CpdBackendHandle initialize_window(CpdWindow cpeed_window) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)malloc(sizeof(CpdDirectXRenderer));
    if (renderer == 0) {
        return 0;
    }

    renderer->device = 0;
    renderer->device_context = 0;
    renderer->swapchain = 0;
    renderer->render_target = 0;

    CpdSize size = window_size(cpeed_window);

    DXGI_SWAP_CHAIN_DESC1 description = {
        .Width = size.width,
        .Height = size.height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .Stereo = FALSE,

        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,

        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 3,
        .Scaling = DXGI_SCALING_NONE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    };

    HRESULT result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, D3D11_SDK_VERSION,
        &renderer->device, &renderer->feature_level, &renderer->device_context);

    if (result != S_OK) {
        free(renderer);
        return 0;
    }

    IDXGIFactory2* factory = 0;
    result = CreateDXGIFactory1(&IID_IDXGIFactory2, &factory);
    if (result != S_OK) {
        shutdown_window(renderer);
        return 0;
    }

    if (compile_platform() == CpdCompilePlatform_UWP) {
        result = IDXGIFactory2_CreateSwapChainForCoreWindow(factory, (IUnknown*)renderer->device, *(IUnknown**)cpeed_window, &description, 0, &renderer->swapchain);
    }
    else {
        result = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)renderer->device, (HWND)cpeed_window, &description, 0, 0, &renderer->swapchain);
    }

    IDXGIFactory2_Release(factory);

    if (result != S_OK) {
        shutdown_window(renderer);
        return 0;
    }

    result = create_render_target(renderer);
    if (result != S_OK) {
        shutdown_window(renderer);
        return 0;
    }

    return renderer;
}

static CpdBackendVersion get_version(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    switch (renderer->feature_level) {
        case D3D_FEATURE_LEVEL_12_2: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_12_1: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_12_0: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_11_1: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_11_0: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_10_1: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_10_0: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_9_3: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_9_2: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        case D3D_FEATURE_LEVEL_9_1: return (CpdBackendVersion) { .major = 12, .minor = 2 };
        default: return (CpdBackendVersion) { .major = 1, .minor = 0 };
    }
}

static bool resize(CpdBackendHandle cpeed_backend, CpdSize new_size) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    ID3D11DeviceContext_Flush(renderer->device_context);

    ID3D11RenderTargetView_Release(renderer->render_target);

    HRESULT result = IDXGISwapChain1_ResizeBuffers(renderer->swapchain, 0, new_size.width, new_size.height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (result != S_OK) {
        return result;
    }

    result = create_render_target(renderer);

    return result == S_OK;
}

static float brightness_red = 1.0f;
static float brightness_green = 1.0f;
static float brightness_blue = 1.0f;
static float brightness_step = 0.05f;

static void input(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window, const CpdInputEvent* input_events, uint32_t input_event_count) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    for (uint32_t i = 0; i < input_event_count; i++) {
        const CpdInputEvent* event = &input_events[i];

        if (event->type == CpdInputEventType_CharInput) {
            uint64_t text = event->data.char_input.character;
            printf("Char input: %s (%d bytes)\n", (char*)&text, event->data.char_input.length);
        }
        else if (event->type == CpdInputEventType_ButtonPress) {
            if (event->data.button_press.pressed) {
                if (event->data.button_press.key_code == CpdKeyCode_Numpad7 && brightness_red < 1.0f) {
                    brightness_red += brightness_step;
                    printf("New red brightness: %.2f\n", brightness_red);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad4 && brightness_red > 0.0f) {
                    brightness_red -= brightness_step;
                    printf("New red brightness: %.2f\n", brightness_red);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad8 && brightness_green < 1.0f) {
                    brightness_green += brightness_step;
                    printf("New green brightness: %.2f\n", brightness_green);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad5 && brightness_green > 0.0f) {
                    brightness_green -= brightness_step;
                    printf("New green brightness: %.2f\n", brightness_green);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad9 && brightness_blue < 1.0f) {
                    brightness_blue += brightness_step;
                    printf("New blue brightness: %.2f\n", brightness_blue);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad6 && brightness_blue > 0.0f) {
                    brightness_blue -= brightness_step;
                    printf("New blue brightness: %.2f\n", brightness_blue);
                }

                DXGI_RGBA color = {
                    .r = brightness_red,
                    .g = brightness_green,
                    .b = brightness_blue,
                    .a = 1.0f
                };
                IDXGISwapChain1_SetBackgroundColor(renderer->swapchain, &color);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Escape) {
                close_window(cpeed_window);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Tab) {
                CpdBackendVersion version = get_version(cpeed_backend);

                printf("DirectX version: %d.%d\n", version.major, version.minor);
            }
        }
        else if (event->type == CpdInputEventType_Clipboard) {
            if (event->data.clipboard.action_type == CpdClipboardActionType_Paste) {
                printf("Paste data\n");
            }
            else if (event->data.clipboard.action_type == CpdClipboardActionType_Copy) {
                printf("Copy data\n");
            }
            else if (event->data.clipboard.action_type == CpdClipboardActionType_Cut) {
                printf("Cut data\n");
            }
        }
        else if (event->type == CpdInputEventType_GamepadButtonPress) {
            if (event->data.gamepad_button_press.pressed) {
                if (event->data.gamepad_button_press.button == CpdGamepadButtonType_StickLeft) {
                    DebugBreak();
                }
            }
        }
        else if (event->type == CpdInputEventType_GamepadStick && event->data.gamepad_stick.stick == CpdGamepadStick_Left) {
            CpdGamepadStickPosition pos = get_gamepad_stick_position(cpeed_window, event->data.gamepad_stick.gamepad_index, event->data.gamepad_stick.stick);

            if (pos.x < 0) {
                pos.x = -pos.x;
            }

            if (pos.y < 0) {
                pos.y = -pos.y;
            }

            brightness_red = pos.x;
            brightness_green = pos.y;
        }
        else if (event->type == CpdInputEventType_GamepadConnect) {
            if (event->data.gamepad_connect.status == CpdGamepadConnectStatus_Connected) {
                printf("Gamepad %d connected.\n", event->data.gamepad_connect.gamepad_index + 1);
            }
            else {
                printf("Gamepad %d disconnected.\n", event->data.gamepad_connect.gamepad_index + 1);
            }
        }
    }
}

static bool should_frame(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window) {
    return true;
}

static bool pre_frame(CpdBackendHandle cpeed_backend) {
    return true;
}

static bool frame(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    const float color[4] = { brightness_red, brightness_green, brightness_blue, 1.0f };

    ID3D11DeviceContext_ClearRenderTargetView(renderer->device_context, renderer->render_target, color);

    return true;
}

static bool post_frame(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    HRESULT result = IDXGISwapChain1_Present(renderer->swapchain, 1, 0);
    
    return result == S_OK;
}

void get_directx_backend_implementation(CpdBackendImplementation* implementation) {
    implementation->type = CpdPlatformBackendFlags_DirectX;
    implementation->initialize_backend = initialize_backend;
    implementation->shutdown_backend = shutdown_backend;
    implementation->initialize_window = initialize_window;
    implementation->shutdown_window = shutdown_window;
    implementation->get_version = get_version;
    implementation->resize = resize;
    implementation->input = input;
    implementation->should_frame = should_frame;
    implementation->pre_frame = pre_frame;
    implementation->frame = frame;
    implementation->post_frame = post_frame;
}
