#define WIN32_LEAN_AND_MEAN

#include <malloc.h>
#include <windows.h>

#include <Cpeed/platform/lock.h>

CpdLock create_lock() {
    LPCRITICAL_SECTION section = (LPCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
    if (section == 0) {
        return 0;
    }

    InitializeCriticalSection(section);

    return (CpdLock)section;
}

void destroy_lock(CpdLock cpeed_lock) {
    DeleteCriticalSection((LPCRITICAL_SECTION)cpeed_lock);
    free(cpeed_lock);
}

bool try_enter_lock(CpdLock cpeed_lock) {
    return TryEnterCriticalSection((LPCRITICAL_SECTION)cpeed_lock) != 0;
}

bool enter_lock(CpdLock cpeed_lock) {
    EnterCriticalSection((LPCRITICAL_SECTION)cpeed_lock);
    return true;
}

void leave_lock(CpdLock cpeed_lock) {
    LeaveCriticalSection((LPCRITICAL_SECTION)cpeed_lock);
}
