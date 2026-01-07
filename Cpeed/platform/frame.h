#pragma once

#include <stdbool.h>

#include "../platform/window.h"
#include "../input.h"

#define INVALID_FRAME_LAYER_HANDLE ((uint32_t)0)

typedef struct CpdFrame {
    CpdVector3 background;
} CpdFrame;

struct CpdFrameLayer;

typedef enum CpdFrameLayerFlags {
    CpdFrameLayerFlags_None = 0,
    CpdFrameLayerFlags_FirstFramePassed = 1 << 0
} CpdFrameLayerFlags;

typedef bool (*CpdFrameLayerLoopFunction)(void*, struct CpdFrameLayer*);
typedef bool (*CpdFrameInputFunction)(void* context, CpdWindow window, CpdFrame*, const CpdInputEvent*);
typedef void (*CpdFrameRenderFunction)(void* context, CpdFrame*);
typedef void (*CpdFrameResizeFunction)(void* context, CpdWindow window, CpdFrame*, CpdSize size);
typedef void (*CpdFrameFirstFrameFunction)(void* context, CpdWindow window, CpdFrame* frame);
typedef void (*CpdFrameAddedFunction)(void* context);
typedef void (*CpdFrameRemoveFunction)(void* context);

#ifdef CPD_IMGUI_AVAILABLE
typedef void (*CpdFrameImGuiFunction)(void* context);
#endif

typedef struct CpdFrameLayerFunctions {
#ifdef CPD_IMGUI_AVAILABLE
    CpdFrameImGuiFunction imgui;
#endif

    CpdFrameInputFunction input;
    CpdFrameRenderFunction render;
    CpdFrameResizeFunction resize;
    CpdFrameFirstFrameFunction first_frame;
    CpdFrameAddedFunction added;
    CpdFrameRemoveFunction remove;
} CpdFrameLayerFunctions;

typedef struct CpdFrameLayer {
    struct CpdFrameLayer* higher;
    struct CpdFrameLayer* underlying;
    uint32_t handle;
    CpdFrameLayerFlags flags;
    void* context;
    CpdFrameLayerFunctions functions;
} CpdFrameLayer;

extern uint32_t add_frame_layer(CpdWindow window, const CpdFrameLayerFunctions* functions, void* context, CpdFrameLayerFlags flags);
extern void remove_frame_layer(CpdWindow window, uint32_t handle);

extern void loop_frame_layers(CpdWindow window, CpdFrameLayerLoopFunction loop, void* context);
