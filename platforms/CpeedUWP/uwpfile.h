#pragma once

#include <Cpeed/platform/file.h>

#include <Windows.Storage.h>

CpdFileResult open_file_from_storage_file(__x_ABI_CWindows_CStorage_CIStorageFile* storage_file, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file);
