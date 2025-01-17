#pragma once

typedef void* CpdWindow;

typedef enum CpdWindowFlags { } CpdWindowFlags;

typedef struct CpdWindowInfo {
    char* title;
    unsigned short width;
    unsigned short height;
    CpdWindowFlags flags;
    CpdWindow* handle;
} CpdWindowInfo;
