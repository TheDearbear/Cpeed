#define WIN32_LEAN_AND_MEAN

#include <Cpeed/platform/thread.h>

#include <malloc.h>
#include <windows.h>

CpdThread create_thread(CpdThreadFunction func, void* context) {
    HANDLE thread = CreateThread(0, 0, func, context, 0, 0);

    return (CpdThread)thread;
}

CpdThread current_thread() {
    return (CpdThread)GetCurrentThread();
}

void stop_current_thread(uint32_t code) {
    ExitThread(code);
}

bool join_thread(CpdThread handle, uint32_t* return_value) {
    DWORD result = WaitForSingleObject((HANDLE)handle, INFINITE);

    if (result == WAIT_OBJECT_0 && return_value != 0) {
        GetExitCodeThread((HANDLE)handle, return_value);
    }

    CloseHandle((HANDLE)handle);
    return result == WAIT_OBJECT_0;
}

void detach_thread(CpdThread handle) {
    CloseHandle((HANDLE)handle);
}
