#include <malloc.h>

#include <Cpeed/platform/frame.h>

#include "winmain.h"

static uint32_t g_new_layer_handle = 1;

uint32_t add_frame_layer(CpdWindow window, const CpdFrameLayerFunctions* functions, void* context, CpdFrameLayerFlags flags) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    CpdFrameLayer* layer = (CpdFrameLayer*)malloc(sizeof(CpdFrameLayer));
    if (layer == 0) {
        return INVALID_FRAME_LAYER_HANDLE;
    }

    layer->higher = 0;
    layer->underlying = data->layers;
    layer->handle = g_new_layer_handle++;
    layer->flags = flags;
    layer->context = context;
    layer->functions = *functions;

    if (data->layers != 0) {
        data->layers->higher = layer;
    }

    data->layers = layer;

    if (layer->functions.added != 0) {
        layer->functions.added(context);
    }

    return layer->handle;
}

typedef struct CpdRemoveLayerArgs {
    WindowExtraData* data;
    uint32_t handle;
} CpdRemoveLayerArgs;

static bool remove_frame_layer_loop(void* context, CpdFrameLayer* layer) {
    CpdRemoveLayerArgs* args = (CpdRemoveLayerArgs*)context;

    if (layer->handle != args->handle) {
        return true;
    }

    if (layer->functions.remove != 0) {
        layer->functions.remove(layer->context);
    }

    CpdFrameLayer* higher = layer->higher;

    if (higher == 0) {
        args->data->layers = layer->underlying;
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

    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    CpdRemoveLayerArgs args = {
        .data = data,
        .handle = handle
    };

    loop_frame_layers(window, remove_frame_layer_loop, &args);
}

void loop_frame_layers(CpdWindow window, CpdFrameLayerLoopFunction loop, void* context) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);
    CpdFrameLayer* underlying = 0;

    for (CpdFrameLayer* layer = data->layers; layer != 0; layer = underlying) {
        underlying = layer->underlying;

        if (!loop(context, layer)) {
            return;
        }
    }
}
