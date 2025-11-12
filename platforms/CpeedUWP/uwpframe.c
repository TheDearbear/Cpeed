#include <malloc.h>

#include <Cpeed/common/frame.h>

#include "uwpmain.h"

static uint32_t g_new_layer_handle = 1;

uint32_t add_frame_layer(CpdWindow window, const CpdFrameLayerFunctions* functions, CpdFrameLayerFlags flags) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    CpdFrameLayer* layer = (CpdFrameLayer*)malloc(sizeof(CpdFrameLayer));
    if (layer == 0) {
        return INVALID_FRAME_LAYER_HANDLE;
    }

    layer->higher = 0;
    layer->underlying = uwp_window->layers;
    layer->handle = g_new_layer_handle++;
    layer->flags = flags;
    layer->functions = *functions;

    if (uwp_window->layers != 0) {
        uwp_window->layers->higher = layer;
    }

    uwp_window->layers = layer;

    return layer->handle;
}

typedef struct CpdRemoveLayerArgs {
    CpdUWPWindow* uwp_window;
    uint32_t handle;
} CpdRemoveLayerArgs;

static bool remove_frame_layer_loop(void* context, CpdFrameLayer* layer) {
    CpdRemoveLayerArgs* args = (CpdRemoveLayerArgs*)context;

    if (layer->handle != args->handle) {
        return true;
    }

    CpdFrameLayer* higher = layer->higher;


    if (higher == 0) {
        args->uwp_window->layers = layer->underlying;
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

    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    CpdRemoveLayerArgs args = {
        .uwp_window = uwp_window,
        .handle = handle
    };

    loop_frame_layers(window, remove_frame_layer_loop, &args);
}

void loop_frame_layers(CpdWindow window, CpdFrameLayerLoopFunction loop, void* context) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;
    CpdFrameLayer* underlying = 0;

    for (CpdFrameLayer* layer = uwp_window->layers; layer != 0; layer = underlying) {
        underlying = layer->underlying;

        if (!loop(context, layer)) {
            return;
        }
    }
}
