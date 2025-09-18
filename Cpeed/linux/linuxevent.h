#pragma once

#include "linuxmain.h"

typedef struct CpdEventDevice {
    struct CpdEventDevice* next;
    struct sd_device* device;
    const char* device_system_path;
    const char* device_name;
    struct libevdev* ev_device;
    int fd;

    int16_t left_stick_x;
    int16_t left_stick_y;

    int16_t right_stick_x;
    int16_t right_stick_y;

    uint16_t left_trigger;
    uint16_t right_trigger;

    uint64_t button_view : 1;
    uint64_t button_menu : 1;
    uint64_t button_dpad_x : 2;
    uint64_t button_dpad_y : 2;
    uint64_t button_left_stick : 1;
    uint64_t button_right_stick : 1;
    uint64_t button_left_shoulder : 1;
    uint64_t button_right_shoulder : 1;
    uint32_t button_x : 1;
    uint32_t button_y : 1;
    uint32_t button_a : 1;
    uint32_t button_b : 1;
} CpdEventDevice;

extern CpdEventDevice* g_event_devices;

bool init_events();

void poll_events(CpdWaylandWindow* wl_window);

void shutdown_events();
