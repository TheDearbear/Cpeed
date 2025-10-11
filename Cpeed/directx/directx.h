#pragma once

#define CINTERFACE
#define COBJMACROS

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

typedef struct CpdDirectXRenderer {
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    IDXGISwapChain1* swapchain;

    ID3D11RenderTargetView* render_target;

    struct CpdFrame* frame;

    D3D_FEATURE_LEVEL feature_level;
} CpdDirectXRenderer;
