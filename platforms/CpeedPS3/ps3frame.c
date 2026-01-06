#include <malloc.h>

#include <Cpeed/platform/frame.h>

#include "ps3main.h"

static uint32_t g_new_layer_handle = 1;

uint32_t add_frame_layer(CpdWindow window, const CpdFrameLayerFunctions* functions, CpdFrameLayerFlags flags) {
    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    CpdFrameLayer* layer = (CpdFrameLayer*)malloc(sizeof(CpdFrameLayer));
    if (layer == 0) {
        return INVALID_FRAME_LAYER_HANDLE;
    }

    layer->higher = 0;
    layer->underlying = ps3_window->layers;
    layer->handle = g_new_layer_handle++;
    layer->flags = flags;
    layer->functions = *functions;

    if (ps3_window->layers != 0) {
        ps3_window->layers->higher = layer;
    }

    ps3_window->layers = layer;

    return layer->handle;
}

typedef struct CpdRemoveLayerArgs {
    CpdPs3Window* ps3_window;
    uint32_t handle;
} CpdRemoveLayerArgs;

static bool remove_frame_layer_loop(void* context, CpdFrameLayer* layer) {
    CpdRemoveLayerArgs* args = (CpdRemoveLayerArgs*)context;

    if (layer->handle != args->handle) {
        return true;
    }

    CpdFrameLayer* higher = layer->higher;


    if (higher == 0) {
        args->ps3_window->layers = layer->underlying;
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

    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    CpdRemoveLayerArgs args = {
        .ps3_window = ps3_window,
        .handle = handle
    };

    loop_frame_layers(window, remove_frame_layer_loop, &args);
}

void loop_frame_layers(CpdWindow window, CpdFrameLayerLoopFunction loop, void* context) {
    CpdPs3Window* ps3_window = (CpdPs3Window*)window;
    CpdFrameLayer* underlying = 0;

    for (CpdFrameLayer* layer = ps3_window->layers; layer != 0; layer = underlying) {
        underlying = layer->underlying;

        if (!loop(context, layer)) {
            return;
        }
    }
}
