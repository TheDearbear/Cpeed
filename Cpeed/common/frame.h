#pragma once

#include <stdbool.h>

#include "../platform/window.h"
#include "../input.h"
#include "math.h"

#define INVALID_FRAME_LAYER_HANDLE ((uint32_t)0)

typedef enum CpdFrameLayerFlags {
    CpdFrameLayerFlags_None
} CpdFrameLayerFlags;

typedef bool (*CpdFrameLayerLoopFunction)(void*, struct CpdFrameLayer*);
typedef bool (*CpdFrameInputFunction)(CpdWindow window, struct CpdFrame*, const CpdInputEvent*);
typedef void (*CpdFrameRenderFunction)(struct CpdFrame*);

typedef struct CpdFrameLayerFunctions {
    CpdFrameInputFunction input;
    CpdFrameRenderFunction render;
} CpdFrameLayerFunctions;

typedef struct CpdFrameLayer {
    struct CpdFrameLayer* higher;
    struct CpdFrameLayer* underlying;
    uint32_t handle;
    CpdFrameLayerFlags flags;
    CpdFrameLayerFunctions functions;
} CpdFrameLayer;

typedef struct CpdFrame {
    CpdVector3 background;
} CpdFrame;

uint32_t add_frame_layer(const CpdFrameLayerFunctions* functions, CpdFrameLayerFlags flags);
void remove_frame_layer(uint32_t handle);

void loop_frame_layers(CpdFrameLayerLoopFunction loop, void* context);
