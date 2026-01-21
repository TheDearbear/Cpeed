#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uintptr_t CpdThread;

typedef uint32_t (*CpdThreadFunction)(void*);

extern CpdThread create_thread(CpdThreadFunction func, void* context);

extern CpdThread current_thread();

extern void stop_current_thread(uint32_t code);

extern bool join_thread(CpdThread handle, uint32_t* return_value);

extern void detach_thread(CpdThread handle);
