#include <malloc.h>
#include <pthread.h>

#include "../platform/lock.h"

CpdLock create_lock() {
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (mutex == 0) {
        return 0;
    }

    if (pthread_mutex_init(mutex, 0) != 0) {
        free(mutex);
        return 0;
    }

    return (CpdLock)mutex;
}

void destroy_lock(CpdLock cpeed_lock) {
    pthread_mutex_destroy((pthread_mutex_t*)cpeed_lock);
    free(cpeed_lock);
}

bool try_enter_lock(CpdLock cpeed_lock) {
    return pthread_mutex_trylock((pthread_mutex_t*)cpeed_lock) == 0;
}

bool enter_lock(CpdLock cpeed_lock) {
    return pthread_mutex_lock((pthread_mutex_t*)cpeed_lock) == 0;
}

void leave_lock(CpdLock cpeed_lock) {
    pthread_mutex_unlock((pthread_mutex_t*)cpeed_lock);
}
