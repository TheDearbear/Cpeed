#pragma once

#include <stdbool.h>

#include "../platform/window.h"
#include "../input.h"
#include "math.h"

#define INVALID_FRAME_LAYER_HANDLE ((uint32_t)0)

typedef struct CpdFrame {
    CpdVector3 background;
} CpdFrame;

struct CpdFrameLayer;

typedef enum CpdFrameLayerFlags {
    CpdFrameLayerFlags_None
} CpdFrameLayerFlags;

typedef bool (*CpdFrameLayerLoopFunction)(void*, struct CpdFrameLayer*);
typedef bool (*CpdFrameInputFunction)(CpdWindow window, CpdFrame*, const CpdInputEvent*);
typedef void (*CpdFrameRenderFunction)(CpdFrame*);
typedef void (*CpdFrameResizeFunction)(CpdWindow window, CpdFrame*, CpdSize size);

#ifdef CPD_IMGUI_AVAILABLE
typedef void (*CpdFrameImGuiFunction)();
#endif

typedef struct CpdFrameLayerFunctions {
#ifdef CPD_IMGUI_AVAILABLE
    CpdFrameImGuiFunction imgui;
#endif

    CpdFrameInputFunction input;
    CpdFrameRenderFunction render;
    CpdFrameResizeFunction resize;
} CpdFrameLayerFunctions;

typedef struct CpdFrameLayer {
    struct CpdFrameLayer* higher;
    struct CpdFrameLayer* underlying;
    uint32_t handle;
    CpdFrameLayerFlags flags;
    CpdFrameLayerFunctions functions;
} CpdFrameLayer;

extern uint32_t add_frame_layer(CpdWindow window, const CpdFrameLayerFunctions* functions, CpdFrameLayerFlags flags);
extern void remove_frame_layer(CpdWindow window, uint32_t handle);

extern void loop_frame_layers(CpdWindow window, CpdFrameLayerLoopFunction loop, void* context);
