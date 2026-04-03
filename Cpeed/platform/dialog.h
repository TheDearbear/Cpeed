#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "file.h"
#include "window.h"

typedef struct CpdFileDialogSelector {
    const char* pattern;
    const char* mime_type;
} CpdFileDialogSelector;

typedef struct CpdFileDialogFilter {
    const char* name;
    CpdFileDialogSelector* selectors;
    size_t selector_count;
} CpdFileDialogFilter;

typedef void* CpdFileDialogHandle;

typedef enum CpdFileDialogResult {
    // File Dialog call was successfull
    CpdFileDialogResult_Success,

    // Passed argument(s) are not valid
    CpdFileDialogResult_InvalidCall,

    // File Dialog was cancelled by the user
    CpdFileDialogResult_Canceled,

    // File Dialogs are not supported
    CpdFileDialogResult_Unsupported,

    // File Dialog call failed due to running out of RAM
    CpdFileDialogResult_OutOfMemory,

    // File Dialog call failed due to internal error
    CpdFileDialogResult_InternalError
} CpdFileDialogResult;

extern bool file_dialog_supported();

extern CpdFileDialogResult request_open_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    bool multiple, bool add_wildcard,
    CpdFileDialogHandle* out_handle);

extern CpdFileDialogResult request_save_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    const char* name, const char* folder, bool add_wildcard,
    CpdFileDialogHandle* out_handle);

extern CpdFileDialogResult request_directory_dialog(
    CpdWindow parent, const char* title,
    CpdFileDialogHandle* out_handle);

extern CpdFileResult open_file_from_dialog_handle(
    CpdFileDialogHandle handle, const char* filename,
    CpdFileOpening opening,
    CpdFileMode mode, CpdFileSharing sharing,
    CpdFile* file);

extern bool wait_for_file_dialog(CpdFileDialogHandle handle, uint64_t timeout, char*** paths, uint32_t* count);

extern void cancel_file_dialog(CpdFileDialogHandle handle);

extern void destroy_file_dialog_handle(CpdFileDialogHandle handle);
