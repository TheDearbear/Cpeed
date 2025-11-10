#include <malloc.h>

#include "../common/frame.h"
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

    free(renderer->frame);

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

static CpdBackendHandle initialize_window(const CpdBackendInfo* info) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)malloc(sizeof(CpdDirectXRenderer));
    if (renderer == 0) {
        return 0;
    }

    CpdFrame* frame = (CpdFrame*)malloc(sizeof(CpdFrame));
    if (frame == 0) {
        return 0;
    }

    frame->background = info->background;

    renderer->window = info->window;
    renderer->device = 0;
    renderer->device_context = 0;
    renderer->swapchain = 0;
    renderer->render_target = 0;
    renderer->frame = frame;

    CpdSize size = window_size(info->window);

    DXGI_SWAP_CHAIN_DESC1 description = {
        .Width = size.width,
        .Height = size.height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .Stereo = FALSE,

        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,

        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 3,
        .Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH,
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
        result = IDXGIFactory2_CreateSwapChainForCoreWindow(factory, (IUnknown*)renderer->device, *(IUnknown**)info->window, &description, 0, &renderer->swapchain);
    }
    else {
        result = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)renderer->device, (HWND)info->window, &description, 0, 0, &renderer->swapchain);
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

    return (CpdBackendVersion) {
        .major = renderer->feature_level >> 12,
        .minor = (renderer->feature_level >> 8) & 0x0F
    };
}

static CpdFrame* get_frame(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    return renderer->frame;
}

static bool get_lowest_frame_layer(void* context, CpdFrameLayer* frame_layer) {
    CpdFrameLayer** output = (CpdFrameLayer**)context;

    *output = frame_layer;

    return true;
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

    CpdFrameLayer* frame_layer = 0;
    loop_frame_layers(renderer->window, get_lowest_frame_layer, &frame_layer);

    while (frame_layer != 0) {
        if (frame_layer->functions.resize != 0) {
            frame_layer->functions.resize(renderer->window, renderer->frame, new_size);
        }

        frame_layer = frame_layer->higher;
    }

    return result == S_OK;
}

static bool should_frame(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window) {
    return true;
}

static bool pre_frame(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    const float color[4] = {
        renderer->frame->background.x,
        renderer->frame->background.y,
        renderer->frame->background.z,
        1.0f
    };

    ID3D11DeviceContext_ClearRenderTargetView(renderer->device_context, renderer->render_target, color);
    ID3D11DeviceContext_OMSetRenderTargets(renderer->device_context, 1, &renderer->render_target, 0);

    return true;
}

static bool frame(CpdBackendHandle cpeed_backend) {
    CpdDirectXRenderer* renderer = (CpdDirectXRenderer*)cpeed_backend;

    CpdFrameLayer* lowest = 0;
    loop_frame_layers(renderer->window, get_lowest_frame_layer, &lowest);

    for (CpdFrameLayer* layer = lowest; layer != 0; layer = layer->higher) {
        if (layer->functions.render != 0) {
            layer->functions.render(renderer->frame);
        }
    }

    // TODO: Actual render

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
    implementation->get_frame = get_frame;
    implementation->resize = resize;
    implementation->should_frame = should_frame;
    implementation->pre_frame = pre_frame;
    implementation->frame = frame;
    implementation->post_frame = post_frame;
}
