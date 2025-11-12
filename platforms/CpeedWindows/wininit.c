#include <Cpeed/platform.h>

#include "winmain.h"
#include "winproc.h"

GameInputCallbackToken token;

static bool add_gamepad_connect_to_queue(WindowExtraData* data, CpdGamepadConnectInputEventData event_data) {
    if (!resize_input_queue_if_need(data, 1)) {
        return false;
    }

    data->input_queue[data->input_queue_size++] = (CpdInputEvent){
        .type = CpdInputEventType_GamepadConnect,
        .modifiers = data->current_key_modifiers,
        .time = get_clock_usec(),
        .data.gamepad_connect = event_data
    };

    return true;
}

static BOOL window_enum_procedure(HWND hWnd, LPARAM lParam) {
    CpdInputEventData* event_data = (CpdInputEventData*)lParam;

    DWORD pid;
    if (GetWindowThreadProcessId(hWnd, &pid) == 0) {
        return TRUE;
    }

    if (pid != GetCurrentProcessId()) {
        return TRUE;
    }

    WindowExtraData* data = GET_EXTRA_DATA(hWnd);
    if (data == 0) {
        return TRUE;
    }

    if (!add_gamepad_connect_to_queue(data, event_data->gamepad_connect)) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    return TRUE;
}

static void announce_gamepad_connect(CpdGamepadConnectStatus status, uint16_t index) {
    CpdInputEventData data = {
        .gamepad_connect.status = status,
        .gamepad_connect.gamepad_index = index
    };

    EnumWindows(window_enum_procedure, (LPARAM)&data);
}

static void callback(GameInputCallbackToken token, void* context, IGameInputDevice* device, uint64_t timestamp, GameInputDeviceStatus status, GameInputDeviceStatus previous_status) {
    if ((status & GameInputDeviceConnected) != 0) {
        CpdGamepad* gamepad = (CpdGamepad*)malloc(sizeof(CpdGamepad));
        if (gamepad == 0) {
            return;
        }

        uint16_t index = 0;
        gamepad->next = 0;
        gamepad->device = device;

        IGameInputReading* reading;
        HRESULT result = IGameInput_GetCurrentReading(g_game_input, GameInputKindGamepad, device, &reading);
        if (FAILED(result)) {
            free(gamepad);
            return;
        }

        if (IGameInputReading_GetGamepadState(reading, &gamepad->last_used_state)) {
            if (g_gamepads == 0) {
                g_gamepads = gamepad;
            }
            else {
                CpdGamepad* current = g_gamepads;
                while (current->next != 0) {
                    current = current->next;
                    index++;
                }

                current->next = gamepad;
                index++;
            }

            announce_gamepad_connect(CpdGamepadConnectStatus_Connected, index);
        }
        else {
            free(gamepad);
        }

        IGameInputReading_Release(reading);
    }
    else {
        CpdGamepad* previous = 0;
        CpdGamepad* current = g_gamepads;
        uint16_t index = 0;

        while (current != 0 && current->device != device) {
            current = current->next;
            index++;
        }

        if (current == 0) {
            return;
        }

        announce_gamepad_connect(CpdGamepadConnectStatus_Disconnected, index);

        if (previous == 0) {
            g_gamepads = current->next;
        }
        else {
            previous->next = current->next;
        }

        free(current);
    }
}

bool initialize_platform() {
    if (QueryPerformanceFrequency(&g_counter_frequency) == 0) {
        return false;
    }

    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wndClassExW = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = window_procedure,
        .hInstance = hInstance,
        .hIcon = LoadIconW(hInstance, L"IDI_ICON1"),
        .hCursor = LoadImageW(NULL, (LPCWSTR)IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
        .lpszClassName = L"Cpeed"
    };

    g_window_class = RegisterClassExW(&wndClassExW);

    if (g_window_class == 0) {
        return false;
    }

    HRESULT result = CoInitialize(NULL);
    if (FAILED(result)) {
        return false;
    }

    result = GameInputCreate(&g_game_input);
    if (FAILED(result)) {
        return false;
    }

    return SUCCEEDED(IGameInput_RegisterDeviceCallback(g_game_input, 0, GameInputKindGamepad, GameInputDeviceAnyStatus, GameInputBlockingEnumeration, 0, callback, &token));
}

void shutdown_platform() {
    if (g_game_input != 0) {
        IGameInput_UnregisterCallback(g_game_input, token, 5000);
        IGameInput_Release(g_game_input);
    }

    CoUninitialize();
    UnregisterClassW((LPCWSTR)g_window_class, NULL);
    g_window_class = 0;
}
