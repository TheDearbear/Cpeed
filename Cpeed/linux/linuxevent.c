#include <errno.h>
#include <fcntl.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <systemd/sd-device.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include "linuxevent.h"
#include "linuxmain.h"
#include "linuxwayland.h"
#include "linuxwindowlist.h"

static sd_device_monitor* g_sd_monitor;
static sd_event* g_sd_event;
CpdEventDevice* g_event_devices;

#define EVENT_DEVNAME "/dev/input/event"
#define FIRST_EVENT_DEVNAME "/dev/input/event0"

static bool is_event_device(const char* devname) {
    return devname != 0 && strncmp(EVENT_DEVNAME, devname, strlen(EVENT_DEVNAME)) == 0 && strcmp(devname, FIRST_EVENT_DEVNAME) >= 0;
}

static bool init_device_from_fd(CpdEventDevice* device, sd_device* sd_device, int fd) {
    int result = sd_device_get_syspath(sd_device, &device->device_system_path);
    if (result < 0) {
        return false;
    }
    
    result = libevdev_new_from_fd(fd, &device->ev_device);
    if (result < 0) {
        printf("Unable to init libevdev for device (%d)\n", -result);
        return false;
    }

    device->device = sd_device;
    device->device_name = libevdev_get_name(device->ev_device);
    device->fd = fd;

    return true;
}

static CpdEventDevice* find_device(const char* device_system_path) {
    CpdEventDevice* current = g_event_devices;

    while (current != 0 && strcmp(current->device_system_path, device_system_path) != 0) {
        current = current->next;
    }

    return current;
}

static void announce_gamepad_connect(CpdGamepadConnectStatus status, uint16_t index) {
    for (CpdWaylandWindowListNode* current = g_windows_list; current != 0; current = current->next) {
        add_gamepad_connect_to_queue(current->window, status, index);
    }
}

static void handle_device(sd_device* device, bool added) {
    const char* devname = 0;
    if (sd_device_get_devname(device, &devname) < 0 || !is_event_device(devname)) {
        return;
    }

    bool is_joystick = false;
    for (const char* value, * prop = sd_device_get_property_first(device, &value); prop != 0; prop = sd_device_get_property_next(device, &value)) {
        if (strcmp(prop, "ID_INPUT_JOYSTICK") == 0 && strcmp(value, "1") >= 0) {
            is_joystick = true;
            break;
        }
    }

    if (!is_joystick) {
        return;
    }

    const char* device_system_path = 0;
    if (sd_device_get_syspath(device, &device_system_path) < 0) {
        return;
    }

    if (added) {
        CpdEventDevice* event_device = find_device(device_system_path);

        if (event_device == 0) {
            CpdEventDevice* event_device = (CpdEventDevice*)malloc(sizeof(CpdEventDevice));
            if (event_device == 0) {
                return;
            }

            int fd = open(devname, O_RDONLY | O_NONBLOCK);
            if (fd == -1) {
                free(event_device);

                if (errno == EACCES) {
                    printf("Unable to read device \"%s\" due to lack of permissions. Try to change file permissions of change user.\n", devname);
                }
                else {
                    printf("Unable to read device \"%s\" due to an error %d\n", devname, errno);
                }

                return;
            }

            if (!init_device_from_fd(event_device, device, fd)) {
                free(event_device);
                close(fd);
                return;
            }

            event_device->next = 0;
            sd_device_ref(device);

            uint16_t index = 0;

            if (g_event_devices == 0) {
                g_event_devices = event_device;
            }
            else {
                CpdEventDevice* current = g_event_devices;
                while (current->next != 0) {
                    current = current->next;
                    index++;
                }

                current->next = event_device;
                index++;
            }

            printf("Joystick added \"%s\" (%s)\n", event_device->device_name, devname);
            announce_gamepad_connect(CpdGamepadConnectStatus_Connected, index);
        }
        else {
            libevdev_free(event_device->ev_device);

            sd_device* old_device = event_device->device;
            if (!init_device_from_fd(event_device, device, event_device->fd)) {
                return;
            }

            sd_device_ref(device);
            sd_device_unref(old_device);

            printf("Joystick reinitialized \"%s\" (%s)\n", event_device->device_name, devname);
        }
    }
    else {
        CpdEventDevice* previous = 0;
        CpdEventDevice* current = g_event_devices;
        uint16_t index = 0;

        while (current != 0 && strcmp(current->device_system_path, device_system_path) != 0) {
            previous = current;
            current = current->next;
            index++;
        }

        if (current == 0) {
            return;
        }

        printf("Joystick removed \"%s\" (%s)\n", current->device_name, devname);
        announce_gamepad_connect(CpdGamepadConnectStatus_Disconnected, index);

        libevdev_free(current->ev_device);
        close(current->fd);
        sd_device_unref(current->device);

        if (previous == 0) {
            g_event_devices = current->next;
            free(current);
        }
        else {
            previous->next = current->next;
            free(current);
        }
    }
}

static int device_monitor_handler(sd_device_monitor* monitor, sd_device* device, void* userdata) {
    sd_device_action_t act;
    if (sd_device_get_action(device, &act) < 0) {
        return 0;
    }

    if (act != SD_DEVICE_ADD && act != SD_DEVICE_REMOVE) {
        return 0;
    }

    handle_device(device, act == SD_DEVICE_ADD);
    return 0;
}

static bool init_connected_devices() {
    sd_device_enumerator* enumerator = 0;
    if (sd_device_enumerator_new(&enumerator) < 0) {
        return false;
    }

    if (sd_device_enumerator_add_match_subsystem(enumerator, "input", 1) < 0) {
        sd_device_enumerator_unref(enumerator);
        return false;
    }

    for (sd_device* device = sd_device_enumerator_get_device_first(enumerator); device != 0; device = sd_device_enumerator_get_device_next(enumerator)) {
        handle_device(device, true);
    }

    sd_device_enumerator_unref(enumerator);

    CpdEventDevice* current = g_event_devices;
    while (current != 0) {
        printf("Device: %s\n", current->device_name);
        current = current->next;
    }

    return true;
}

bool init_events() {
    if (sd_event_default(&g_sd_event) < 0) {
        return false;
    }

    if (sd_device_monitor_new(&g_sd_monitor) < 0) {
        return false;
    }

    if (sd_device_monitor_attach_event(g_sd_monitor, g_sd_event) < 0) {
        return false;
    }

    if (sd_device_monitor_filter_add_match_subsystem_devtype(g_sd_monitor, "input", 0) < 0) {
        return false;
    }

    if (!init_connected_devices()) {
        return false;
    }

    if (sd_device_monitor_start(g_sd_monitor, device_monitor_handler, 0) < 0) {
        return false;
    }

    return true;
}

void shutdown_events() {
    for (CpdEventDevice* current = g_event_devices; current != 0;) {
        CpdEventDevice* next = current->next;

        free(current);

        current = next;
    }

    g_event_devices = 0;

    if (g_sd_monitor != 0) {
        sd_device_monitor_stop(g_sd_monitor);
        sd_device_monitor_detach_event(g_sd_monitor);
        sd_device_monitor_unref(g_sd_monitor);

        g_sd_monitor = 0;
    }

    if (g_sd_event != 0) {
        sd_event_unref(g_sd_event);
        g_sd_event = 0;
    }
}

static CpdGamepadButtonType map_code_to_button(__u16 code) {
    switch (code) {
        case BTN_SELECT: return CpdGamepadButtonType_View;
        case BTN_START:  return CpdGamepadButtonType_Menu;
        case BTN_THUMBL: return CpdGamepadButtonType_StickLeft;
        case BTN_THUMBR: return CpdGamepadButtonType_StickRight;
        case BTN_TL:     return CpdGamepadButtonType_ShoulderLeft;
        case BTN_TR:     return CpdGamepadButtonType_ShoulderRight;
        case BTN_NORTH:  return CpdGamepadButtonType_X;
        case BTN_WEST:   return CpdGamepadButtonType_Y;
        case BTN_SOUTH:  return CpdGamepadButtonType_A;
        case BTN_EAST:   return CpdGamepadButtonType_B;
        default:         return CpdGamepadButtonType_Invalid;
    }
}

void poll_events(CpdWaylandWindow* wl_window) {
    if (g_sd_event == 0) {
        return;
    }

    if ((sd_event_prepare(g_sd_event) != 0 || sd_event_wait(g_sd_event, 0) != 0) && sd_event_dispatch(g_sd_event) == 0) {
        shutdown_events();
        return;
    }

    uint16_t index = 0;
    for (CpdEventDevice* current = g_event_devices; current != 0; current = current->next) {
        int result;

        int16_t left_stick_x = current->left_stick_x;
        int16_t left_stick_y = current->left_stick_y;

        int16_t right_stick_x = current->right_stick_x;
        int16_t right_stick_y = current->right_stick_y;

        do {
            struct input_event ev;
            result = libevdev_next_event(current->ev_device, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (result == 0) {
                if (ev.type == EV_ABS) {
                    switch (ev.code) {
                        case ABS_X:
                            left_stick_x = (int16_t)ev.value;
                            break;

                        case ABS_Y:
                            left_stick_y = (int16_t)ev.value;
                            break;

                        case ABS_Z:
                            current->left_trigger = (uint16_t)ev.value;
                            add_gamepad_trigger_to_queue(wl_window, CpdGamepadTrigger_Left, index);
                            break;

                        case ABS_RX:
                            right_stick_x = (int16_t)ev.value;
                            break;

                        case ABS_RY:
                            right_stick_y = (int16_t)ev.value;
                            break;

                        case ABS_RZ:
                            current->right_trigger = (uint16_t)ev.value;
                            add_gamepad_trigger_to_queue(wl_window, CpdGamepadTrigger_Right, index);
                            break;

                        case ABS_HAT0X:
                            if (ev.value + 1 != current->button_dpad_x) {
                                if (ev.value == -1) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadLeft, index, true);
                                }
                                else if (current->button_dpad_x == 0) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadLeft, index, false);
                                }
                                else if (ev.value == 1) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadRight, index, true);
                                }
                                else if (current->button_dpad_x == 2) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadRight, index, false);
                                }

                                current->button_dpad_x = ev.value + 1;
                            }
                            break;

                        case ABS_HAT0Y:
                            if (ev.value + 1 != current->button_dpad_y) {
                                if (ev.value == -1) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadUp, index, true);
                                }
                                else if (current->button_dpad_y == 0) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadUp, index, false);
                                }
                                else if (ev.value == 1) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadDown, index, true);
                                }
                                else if (current->button_dpad_y == 2) {
                                    add_gamepad_button_press_to_queue(wl_window, CpdGamepadButtonType_DPadDown, index, false);
                                }

                                current->button_dpad_y = ev.value + 1;
                            }
                            break;
                    }
                }
                else if (ev.type == EV_KEY) {
                    CpdGamepadButtonType mapped = map_code_to_button(ev.code);

                    if (mapped != CpdGamepadButtonType_Invalid) {
                        add_gamepad_button_press_to_queue(wl_window, mapped, index, ev.value != 0);
                    }
                }
            }
        } while (result == LIBEVDEV_READ_STATUS_SUCCESS || result == LIBEVDEV_READ_STATUS_SYNC);

        if (left_stick_x != current->left_stick_x || left_stick_y != current->left_stick_y) {
            add_gamepad_stick_to_queue(wl_window, CpdGamepadStick_Left, index);
            current->left_stick_x = left_stick_x;
            current->left_stick_y = left_stick_y;
        }

        if (right_stick_x != current->right_stick_x || right_stick_y != current->right_stick_y) {
            add_gamepad_stick_to_queue(wl_window, CpdGamepadStick_Right, index);
            current->right_stick_x = right_stick_x;
            current->right_stick_y = right_stick_y;
        }

        index++;
    }
}
