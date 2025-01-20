#pragma once

typedef void* CpdWindow;

// Reserverd for future use
typedef unsigned int CpdWindowFlags;

typedef struct CpdWindowInfo {
    char* title;
    unsigned short width;
    unsigned short height;
    CpdWindowFlags flags;
    CpdWindow* handle;
} CpdWindowInfo;
