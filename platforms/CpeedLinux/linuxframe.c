#include <malloc.h>

#include <Cpeed/platform/frame.h>

#include "linuxmain.h"

static uint32_t g_new_layer_handle = 1;

uint32_t add_frame_layer(CpdWindow window, const CpdFrameLayerFunctions* functions, CpdFrameLayerFlags flags) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    CpdFrameLayer* layer = (CpdFrameLayer*)malloc(sizeof(CpdFrameLayer));
    if (layer == 0) {
        return INVALID_FRAME_LAYER_HANDLE;
    }

    layer->higher = 0;
    layer->underlying = wl_window->layers;
    layer->handle = g_new_layer_handle++;
    layer->flags = flags;
    layer->functions = *functions;

    if (wl_window->layers != 0) {
        wl_window->layers->higher = layer;
    }

    wl_window->layers = layer;

    return layer->handle;
}

typedef struct CpdRemoveLayerArgs {
    CpdWaylandWindow* wl_window;
    uint32_t handle;
} CpdRemoveLayerArgs;

static bool remove_frame_layer_loop(void* context, CpdFrameLayer* layer) {
    CpdRemoveLayerArgs* args = (CpdRemoveLayerArgs*)context;

    if (layer->handle != args->handle) {
        return true;
    }

    CpdFrameLayer* higher = layer->higher;


    if (higher == 0) {
        args->wl_window->layers = layer->underlying;
    }
    else {
        higher->underlying = layer->underlying;
    }

    if (layer->underlying != 0) {
        layer->underlying->higher = higher;
    }

    free(layer);
    return false;
}

void remove_frame_layer(CpdWindow window, uint32_t handle) {
    if (handle == INVALID_FRAME_LAYER_HANDLE) {
        return;
    }

    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    CpdRemoveLayerArgs args = {
        .wl_window = wl_window,
        .handle = handle
    };

    loop_frame_layers(window, remove_frame_layer_loop, &args);
}

void loop_frame_layers(CpdWindow window, CpdFrameLayerLoopFunction loop, void* context) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;
    CpdFrameLayer* underlying = 0;

    for (CpdFrameLayer* layer = wl_window->layers; layer != 0; layer = underlying) {
        underlying = layer->underlying;

        if (!loop(context, layer)) {
            return;
        }
    }
}
