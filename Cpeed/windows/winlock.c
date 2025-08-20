#define WIN32_LEAN_AND_MEAN

#include <malloc.h>
#include <windows.h>

#include "../platform/lock.h"

extern CpdLock create_lock() {
    LPCRITICAL_SECTION section = (LPCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
    if (section == 0) {
        return 0;
    }

    InitializeCriticalSection(section);

    return (CpdLock)section;
}

extern void destroy_lock(CpdLock cpeed_lock) {
    DeleteCriticalSection((LPCRITICAL_SECTION)cpeed_lock);
    free(cpeed_lock);
}

extern bool try_enter_lock(CpdLock cpeed_lock) {
    return TryEnterCriticalSection((LPCRITICAL_SECTION)cpeed_lock) != 0;
}

extern bool enter_lock(CpdLock cpeed_lock) {
    EnterCriticalSection((LPCRITICAL_SECTION)cpeed_lock);
    return true;
}

extern void leave_lock(CpdLock cpeed_lock) {
    LeaveCriticalSection((LPCRITICAL_SECTION)cpeed_lock);
}
