#pragma once

#include <Cpeed/platform/file.h>

#include "winmain.h"

typedef struct CpdWindowsFile {
    HANDLE handle;
    char* path;
    CpdFileMode mode;
    CpdFileSharing sharing;
} CpdWindowsFile;

CpdFileResult open_file_wide(LPCWSTR path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file);
