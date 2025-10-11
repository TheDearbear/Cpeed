#include <malloc.h>

#include "frame.h"

static CpdFrameLayer* g_layers = 0;
static uint32_t g_new_layer_handle = 1;

uint32_t add_frame_layer(const CpdFrameLayerFunctions* functions, CpdFrameLayerFlags flags) {
    CpdFrameLayer* layer = (CpdFrameLayer*)malloc(sizeof(CpdFrameLayer));
    if (layer == 0) {
        return INVALID_FRAME_LAYER_HANDLE;
    }

    layer->higher = 0;
    layer->underlying = g_layers;
    layer->handle = g_new_layer_handle++;
    layer->flags = flags;
    layer->functions = *functions;

    if (g_layers != 0) {
        g_layers->higher = layer;
    }

    g_layers = layer;

    return layer->handle;
}

static bool remove_frame_layer_loop(void* context, CpdFrameLayer* layer) {
    uint32_t handle = *(uint32_t*)context;

    if (layer->handle != handle) {
        return true;
    }

    CpdFrameLayer* higher = layer->higher;


    if (higher == 0) {
        g_layers = layer->underlying;
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

void remove_frame_layer(uint32_t handle) {
    loop_frame_layers(remove_frame_layer_loop, &handle);
}

void loop_frame_layers(CpdFrameLayerLoopFunction loop, void* context) {
    CpdFrameLayer* underlying = 0;

    for (CpdFrameLayer* layer = g_layers; layer != 0; layer = underlying) {
        underlying = layer->underlying;

        if (!loop(context, layer)) {
            return;
        }
    }
}
