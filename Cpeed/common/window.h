#pragma once

#include "general.h"

typedef void* CpdWindow;

// Reserved for future use
typedef unsigned int CpdWindowFlags;

typedef struct CpdWindowInfo {
    char* title;
    CpdSize size;
    CpdWindowFlags flags;
    CpdWindow* handle;
} CpdWindowInfo;
