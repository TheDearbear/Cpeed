#include <rsx/rsx.h>
#include <stdlib.h>
#include <string.h>
#include <sys/systime.h>
#include <sysutil/video.h>
#include <malloc.h>

#include <Cpeed/platform/logging.h>

#include "backend.h"
#include "device.h"
#include "ppu-lv2-game.h"

#ifdef CPD_IMGUI_AVAILABLE
#include <Cpeed/common/imgui/imgui_impl_cpeed.h>
#include "imgui/dcimgui_impl_rsx.h"
#endif

static gcmContextData* g_context;
static void* g_memory;

static void shutdown_backend() {
    free(g_memory);

    if (g_context != 0) {
        rsxFinish(g_context, 0);
    }
}

static bool initialize_backend() {
    g_memory = memalign(1024 * 1024, HOST_SIZE);
    if (g_memory == 0) {
        return false;
    }

    return rsxInit(&g_context, CB_SIZE, HOST_SIZE, g_memory) == 0;
}

static void shutdown_window(CpdBackendHandle cpeed_backend) {
    CpdOutputDevice* cpeed_device = (CpdOutputDevice*)cpeed_backend;

#ifdef CPD_IMGUI_AVAILABLE
    if (ImGui_GetIO()->BackendRendererUserData != 0) {
        cImGui_ImplRSX_Shutdown();
    }
#endif

    rsxFree(cpeed_device->base_address.color);
    rsxFree(cpeed_device->base_address.depth);

    free(cpeed_device->frame);
    free(cpeed_device);
}

static CpdBackendHandle initialize_window(const CpdBackendInfo* info) {
    videoState video_state;
    videoResolution video_resolution;

    s32 result = videoGetState(VIDEO_PRIMARY, 0, &video_state);
    if (result < 0) {
        return 0;
    }

    result = videoGetResolution(video_state.displayMode.resolution, &video_resolution);
    if (result < 0) {
        return 0;
    }

    videoConfiguration video_configuration;
    memset(&video_configuration, 0, sizeof(video_configuration));

    u32 color_depth = 4;
    u32 depth_depth = 4;
    u32 color_pitch = video_resolution.width * color_depth;
    u32 depth_pitch = video_resolution.width * depth_depth;

    video_configuration.resolution = video_state.displayMode.resolution;
    video_configuration.format = VIDEO_BUFFER_FORMAT_XRGB;
    video_configuration.aspect = VIDEO_ASPECT_AUTO;
    video_configuration.pitch = color_pitch;

    result = videoConfigure(VIDEO_PRIMARY, &video_configuration, 0, 0);
    if (result < 0) {
        return 0;
    }

    u32 color_size = color_pitch * video_resolution.height;
    u32 depth_size = depth_pitch * video_resolution.height;

    void* color_buffer = rsxMemalign(64, COLOR_BUFFER_NUM * color_size);
    if (color_buffer == 0) {
        return 0;
    }

    void* depth_buffer = rsxMemalign(64, COLOR_BUFFER_NUM * depth_size);
    if (depth_buffer == 0) {
        rsxFree(color_buffer);
        return 0;
    }

    CpdOutputDevice* cpeed_device = (CpdOutputDevice*)malloc(sizeof(CpdOutputDevice));
    if (cpeed_device == 0) {
        rsxFree(color_buffer);
        rsxFree(depth_buffer);
        return 0;
    }

    for (int i = 0; i < COLOR_BUFFER_NUM; i++) {
        cpeed_device->addresses[i].color = (void*)((u32)color_buffer + (i * color_size));
        rsxAddressToOffset(cpeed_device->addresses[i].color, &cpeed_device->offsets[i].color);

        cpeed_device->addresses[i].depth = (void*)((u32)depth_buffer + (i * depth_size));
        rsxAddressToOffset(cpeed_device->addresses[i].depth, &cpeed_device->offsets[i].depth);

        gcmSetDisplayBuffer(i, cpeed_device->offsets[i].color, color_pitch, video_resolution.width, video_resolution.height);
    }

    CpdFrame* frame = (CpdFrame*)malloc(sizeof(CpdFrame));
    if (frame == 0) {
        free(cpeed_device);
        rsxFree(color_buffer);
        rsxFree(depth_buffer);
        return 0;
    }

#ifdef CPD_IMGUI_AVAILABLE
    if (!cImGui_ImplRSX_Init(g_context)) {
        shutdown_window(cpeed_device);
        return 0;
    }
#endif

    frame->background = info->background;

    cpeed_device->video_out = VIDEO_PRIMARY;
    cpeed_device->device_index = 0;

    cpeed_device->size = (CpdSize) {
        .width = video_resolution.width,
        .height = video_resolution.height
    };

    cpeed_device->base_address.color = color_buffer;
    cpeed_device->base_address.depth = depth_buffer;

    cpeed_device->color_pitch = color_pitch;
    cpeed_device->depth_pitch = depth_pitch;

    cpeed_device->frame_index = 0;
    cpeed_device->frame = frame;

    return cpeed_device;
}

static CpdBackendVersion get_version(CpdBackendHandle cpeed_backend) {
    s32 version = sys_game_get_system_sw_version();

    return (CpdBackendVersion) {
        .major = version / 10000,
        .minor = (version % 10000) / 100
    };
}

static CpdFrame* get_frame(CpdBackendHandle cpeed_backend) {
    CpdOutputDevice* cpeed_device = (CpdOutputDevice*)cpeed_backend;

    return cpeed_device->frame;
}

static bool resize(CpdBackendHandle cpeed_backend, CpdSize new_size) {
    CpdOutputDevice* cpeed_device = (CpdOutputDevice*)cpeed_backend;

    cpeed_device->size = new_size;

    return true;
}

static bool should_frame(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window) {
    return true;
}

static bool pre_frame(CpdBackendHandle cpeed_backend) {
    CpdOutputDevice* cpeed_device = (CpdOutputDevice*)cpeed_backend;

    gcmSurface surface = {
        .type = GCM_SURFACE_TYPE_LINEAR,
        .antiAlias = GCM_SURFACE_CENTER_1,
        .colorFormat = GCM_SURFACE_A8R8G8B8,
        .colorTarget = GCM_SURFACE_TARGET_0,
        .colorLocation[0] = GCM_LOCATION_RSX,
        .colorOffset[0] = cpeed_device->offsets[cpeed_device->frame_index].color,
        .colorPitch[0] = cpeed_device->color_pitch,

        .depthFormat = GCM_SURFACE_ZETA_Z24S8,
        .depthLocation = GCM_LOCATION_RSX,
        ._pad = { 0, 0 },
        .depthOffset = cpeed_device->offsets[cpeed_device->frame_index].depth,
        .depthPitch = cpeed_device->depth_pitch,

        .width = cpeed_device->size.width,
        .height = cpeed_device->size.height,
        .x = 0,
        .y = 0
    };

    rsxSetSurface(g_context, &surface);

    f32 min = 0.0f;
    f32 max = 1.0f;

    f32 scale[4] = {
        cpeed_device->size.width * 0.5f,
        cpeed_device->size.height * -0.5f,
        (max - min) * 0.5f,
        0.0f
    };

    f32 offset[4] = {
        scale[0],
        -scale[1],
        (max + min) * 0.5f,
        0.0f
    };

    rsxSetViewport(g_context, 0, 0, cpeed_device->size.width, cpeed_device->size.height, min, max, scale, offset);
    
    rsxSetDepthTestEnable(g_context, GCM_TRUE);
    rsxSetClearColor(g_context, 0xFFFFFFFFu);
    rsxSetClearDepthStencil(g_context, 0xFFFFFF00u);

    rsxClearSurface(g_context, GCM_CLEAR_M);
    
    return true;
}

static bool frame(CpdBackendHandle cpeed_backend) {
    return true;
}

static bool post_frame(CpdBackendHandle cpeed_backend) {
    CpdOutputDevice* cpeed_device = (CpdOutputDevice*)cpeed_backend;

    while (gcmGetFlipStatus() != 0) {
        sysUsleep(250);
    }

    gcmResetFlipStatus();

    gcmSetFlip(g_context, cpeed_device->frame_index);
    rsxFlushBuffer(g_context);

    gcmSetWaitFlip(g_context);

    cpeed_device->frame_index = (cpeed_device->frame_index + 1) % COLOR_BUFFER_NUM;

    return true;
}

void get_rsx_backend_implementation(CpdBackendImplementation* implementation) {
    implementation->type = CpdPlatformBackendFlags_RSX;
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
