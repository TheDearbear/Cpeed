#pragma once

#include "common/general.h"

typedef void* CpdWindow;

// Reserved for future use
typedef unsigned int CpdWindowFlags;

typedef struct CpdWindowInfo {
    char* title;
    CpdSize size;
    CpdWindowFlags flags;
    CpdInputMode input_mode;
} CpdWindowInfo;
