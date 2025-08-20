#pragma once

#include <stdbool.h>

typedef void* CpdLock;

// Allocates new lock object. It will return null if something goes wrong.
extern CpdLock create_lock();

// Destroys and frees lock object.
extern void destroy_lock(CpdLock cpeed_lock);

// Tries to acquire lock. Returns false in case of failure or if it is already locked.
extern bool try_enter_lock(CpdLock cpeed_lock);

// Acquires lock. Also waits for unlock if it is already locked. Returns false in case of failure.
extern bool enter_lock(CpdLock cpeed_lock);

// Unlocks access to lock object.
extern void leave_lock(CpdLock cpeed_lock);
