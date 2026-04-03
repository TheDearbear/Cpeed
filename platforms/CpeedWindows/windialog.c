#define CINTERFACE

#include <Cpeed/platform/dialog.h>
#include <Cpeed/platform/lock.h>
#include <Cpeed/platform/logging.h>
#include <Cpeed/platform/thread.h>
#include <Cpeed/platform.h>

#include <malloc.h>

#include "winfile.h"
#include <shobjidl.h>

#define WAIT_SLEEP_TIME_MSEC 100

static const WCHAR file_pattern_separator = L';';

typedef struct CpdFileDialogInternal {
    CpdLock lock;
    IFileDialog* dialog;
    HWND hWnd;
    CpdThread thread;
    HRESULT result;
} CpdFileDialogInternal;

bool file_dialog_supported() {
    return true;
}

static uint32_t dialog_thread(void* context) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)context;

    fdi->result = IFileDialog_Show(fdi->dialog, fdi->hWnd);

    leave_lock(fdi->lock);
}

CpdFileDialogResult request_open_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    bool multiple, bool add_wildcard,
    CpdFileDialogHandle* out_handle
) {
    IFileOpenDialog* dialog = 0;
    HRESULT result = CoCreateInstance(&CLSID_FileOpenDialog, 0, CLSCTX_ALL, &IID_IFileOpenDialog, (LPVOID*)&dialog);

    if (FAILED(result)) {
        return CpdFileDialogResult_Unsupported;
    }

    if (out_handle == 0) {
        IFileOpenDialog_Release(dialog);
        return CpdFileDialogResult_InvalidCall;
    }

    if (filters == 0 && filter_count > 0) {
        IFileOpenDialog_Release(dialog);
        return CpdFileDialogResult_InvalidCall;
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        IFileOpenDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        destroy_lock(lock);
        IFileOpenDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;
    fdi->dialog = (IFileDialog*)dialog;
    fdi->hWnd = (HWND)parent;

    if (title != 0) {
        int size = MultiByteToWideChar(CP_UTF8, 0, title, -1, 0, 0);
        if (size == 0) {
            destroy_lock(lock);
            IFileOpenDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        WCHAR* wtitle = (WCHAR*)malloc(size * sizeof(WCHAR));
        if (wtitle == 0) {
            destroy_lock(lock);
            IFileOpenDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_OutOfMemory;
        }

        MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, size);

        IFileOpenDialog_SetTitle(dialog, wtitle);
        free(wtitle);
    }

    if (multiple) {
        FILEOPENDIALOGOPTIONS options;
        IFileOpenDialog_GetOptions(dialog, &options);

        options |= FOS_ALLOWMULTISELECT;
        IFileOpenDialog_SetOptions(dialog, options);
    }

    if (filter_count != 0 || add_wildcard) {
        size_t file_type_count = filter_count + (add_wildcard != 0);
        COMDLG_FILTERSPEC* file_types = (COMDLG_FILTERSPEC*)malloc(file_type_count * sizeof(COMDLG_FILTERSPEC));
        if (file_types == 0) {
            destroy_lock(lock);
            IFileOpenDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_OutOfMemory;
        }

        for (UINT i = 0; i < filter_count; i++) {
            int name_len = MultiByteToWideChar(CP_UTF8, 0, filters[i].name, -1, 0, 0);
            if (name_len == 0) {
                for (UINT j = 0; j < i; j++) {
                    free((void*)file_types[j].pszName);
                }

                free(file_types);
                destroy_lock(lock);
                IFileOpenDialog_Release(dialog);
                free(fdi);
                return CpdFileDialogResult_InvalidCall;
            }

            size_t selector_count = filters[i].selector_count;

            // Should be 'selector_count - 1' instead of 'selector_count' but we also
            // need null terminator at the end of array, so '- 1' cancels out
            int pattern_len = sizeof(file_pattern_separator) * selector_count;

            for (size_t j = 0; j < selector_count; j++) {
                const char* pattern = filters[i].selectors[j].pattern;
                size_t c_pattern_len = strlen(pattern);

                int wide_pattern_len = MultiByteToWideChar(CP_UTF8, 0, pattern, c_pattern_len, 0, 0);
                if (wide_pattern_len == 0) {
                    for (UINT j = 0; j < i; j++) {
                        free((void*)file_types[j].pszName);
                    }

                    free(file_types);
                    destroy_lock(lock);
                    IFileOpenDialog_Release(dialog);
                    free(fdi);
                    return CpdFileDialogResult_InvalidCall;
                }

                pattern_len += wide_pattern_len;
            }

            size_t filter_size = (size_t)name_len + pattern_len;
            WCHAR* filter = (WCHAR*)malloc(filter_size * sizeof(WCHAR));
            if (filter == 0) {
                for (UINT j = 0; j < i; j++) {
                    free((void*)file_types[j].pszName);
                }

                free(file_types);
                destroy_lock(lock);
                IFileOpenDialog_Release(dialog);
                free(fdi);
                return CpdFileDialogResult_OutOfMemory;
            }

            int offset = MultiByteToWideChar(CP_UTF8, 0, filters[i].name, -1, filter, filter_size);
            if (offset == 0) {
                for (UINT j = 0; j < i; j++) {
                    free((void*)file_types[j].pszName);
                }

                free(filter);
                free(file_types);
                destroy_lock(lock);
                IFileOpenDialog_Release(dialog);
                free(fdi);
                return CpdFileDialogResult_OutOfMemory;
            }

            filter[offset++] = 0;
            int pattern_offset = offset;

            for (UINT j = 0; j < selector_count; j++) {
                if (j != 0) {
                    filter[offset++] = file_pattern_separator;
                }

                const char* pattern = filters[i].selectors[j].pattern;
                size_t c_pattern_len = strlen(pattern);

                int add_offset = MultiByteToWideChar(CP_UTF8, 0, pattern, c_pattern_len, &filter[offset], filter_size - offset);
                if (add_offset == 0) {
                    for (UINT j = 0; j < i; j++) {
                        free((void*)file_types[j].pszName);
                    }

                    free(filter);
                    free(file_types);
                    destroy_lock(lock);
                    IFileOpenDialog_Release(dialog);
                    free(fdi);
                    return CpdFileDialogResult_OutOfMemory;
                }

                offset += add_offset;
            }

            filter[offset] = 0;
            file_types[i].pszName = filter;
            file_types[i].pszSpec = &filter[pattern_offset];
        }

        if (add_wildcard) {
            file_types[file_type_count - 1] = (COMDLG_FILTERSPEC) {
                .pszName = L"All files",
                .pszSpec = L"*"
            };
        }

        IFileOpenDialog_SetFileTypes(dialog, file_type_count, file_types);

        for (UINT i = 0; i < filter_count; i++) {
            free((void*)file_types[i].pszName);
        }

        free(file_types);
    }

    fdi->thread = create_thread(dialog_thread, fdi);

    if (fdi->thread == 0) {
        destroy_lock(lock);
        IFileOpenDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    *out_handle = (CpdFileDialogHandle)fdi;

    return CpdFileDialogResult_Success;
}

CpdFileDialogResult request_save_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    const char* name, const char* folder, bool add_wildcard,
    CpdFileDialogHandle* out_handle
) {
    IFileSaveDialog* dialog = 0;
    HRESULT result = CoCreateInstance(&CLSID_FileSaveDialog, 0, CLSCTX_ALL, &IID_IFileSaveDialog, (LPVOID*)&dialog);

    if (FAILED(result)) {
        return CpdFileDialogResult_Unsupported;
    }

    if (out_handle == 0) {
        IFileSaveDialog_Release(dialog);
        return CpdFileDialogResult_InvalidCall;
    }

    if (filters == 0 && filter_count > 0) {
        IFileSaveDialog_Release(dialog);
        return CpdFileDialogResult_InvalidCall;
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        IFileSaveDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        destroy_lock(lock);
        IFileSaveDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;
    fdi->dialog = (IFileDialog*)dialog;
    fdi->hWnd = (HWND)parent;

    if (title != 0) {
        int size = MultiByteToWideChar(CP_UTF8, 0, title, -1, 0, 0);
        if (size == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        WCHAR* wtitle = (WCHAR*)malloc(size * sizeof(WCHAR));
        if (wtitle == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_OutOfMemory;
        }

        MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, size);

        IFileSaveDialog_SetTitle(dialog, wtitle);
        free(wtitle);
    }

    if (name != 0) {
        int size = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);
        if (size == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        WCHAR* wide_name = (WCHAR*)malloc(size * sizeof(WCHAR));
        if (wide_name == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        MultiByteToWideChar(CP_UTF8, 0, name, -1, wide_name, size);
        IFileSaveDialog_SetFileName(dialog, wide_name);
        free(wide_name);
    }

    if (folder != 0) {
        int size = MultiByteToWideChar(CP_UTF8, 0, folder, -1, 0, 0);
        if (size == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        WCHAR* wide_folder = (WCHAR*)malloc(size * sizeof(WCHAR));
        if (wide_folder == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        MultiByteToWideChar(CP_UTF8, 0, folder, -1, wide_folder, size);

        IShellItem* shell_item = 0;
        result = SHCreateItemFromParsingName(wide_folder, 0, &IID_IShellItem, &shell_item);
        free(wide_folder);

        if (FAILED(result)) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        IFileSaveDialog_SetFolder(dialog, shell_item);
        IShellItem_Release(shell_item);
    }

    if (filter_count != 0 || add_wildcard) {
        size_t file_type_count = filter_count + (add_wildcard != 0);
        COMDLG_FILTERSPEC* file_types = (COMDLG_FILTERSPEC*)malloc(file_type_count * sizeof(COMDLG_FILTERSPEC));
        if (file_types == 0) {
            destroy_lock(lock);
            IFileSaveDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_OutOfMemory;
        }

        for (UINT i = 0; i < filter_count; i++) {
            int name_len = MultiByteToWideChar(CP_UTF8, 0, filters[i].name, -1, 0, 0);
            if (name_len == 0) {
                for (UINT j = 0; j < i; j++) {
                    free((void*)file_types[j].pszName);
                }

                free(file_types);
                destroy_lock(lock);
                IFileSaveDialog_Release(dialog);
                free(fdi);
                return CpdFileDialogResult_InvalidCall;
            }

            size_t selector_count = filters[i].selector_count;

            // Should be 'selector_count - 1' instead of 'selector_count' but we also
            // need null terminator at the end of array, so '- 1' cancels out
            int pattern_len = sizeof(file_pattern_separator) * selector_count;

            for (size_t j = 0; j < selector_count; j++) {
                const char* pattern = filters[i].selectors[j].pattern;
                size_t c_pattern_len = strlen(pattern);

                int wide_pattern_len = MultiByteToWideChar(CP_UTF8, 0, pattern, c_pattern_len, 0, 0);
                if (wide_pattern_len == 0) {
                    for (UINT j = 0; j < i; j++) {
                        free((void*)file_types[j].pszName);
                    }

                    free(file_types);
                    destroy_lock(lock);
                    IFileSaveDialog_Release(dialog);
                    free(fdi);
                    return CpdFileDialogResult_InvalidCall;
                }

                pattern_len += wide_pattern_len;
            }

            size_t filter_size = (size_t)name_len + pattern_len;
            WCHAR* filter = (WCHAR*)malloc(filter_size * sizeof(WCHAR));
            if (filter == 0) {
                for (UINT j = 0; j < i; j++) {
                    free((void*)file_types[j].pszName);
                }

                free(file_types);
                destroy_lock(lock);
                IFileSaveDialog_Release(dialog);
                free(fdi);
                return CpdFileDialogResult_OutOfMemory;
            }

            int offset = MultiByteToWideChar(CP_UTF8, 0, filters[i].name, -1, filter, filter_size);
            if (offset == 0) {
                for (UINT j = 0; j < i; j++) {
                    free((void*)file_types[j].pszName);
                }

                free(filter);
                free(file_types);
                destroy_lock(lock);
                IFileSaveDialog_Release(dialog);
                free(fdi);
                return CpdFileDialogResult_OutOfMemory;
            }

            filter[offset++] = 0;
            int pattern_offset = offset;

            for (UINT j = 0; j < selector_count; j++) {
                if (j != 0) {
                    filter[offset++] = file_pattern_separator;
                }

                const char* pattern = filters[i].selectors[j].pattern;
                size_t c_pattern_len = strlen(pattern);

                int add_offset = MultiByteToWideChar(CP_UTF8, 0, pattern, c_pattern_len, &filter[offset], filter_size - offset);
                if (add_offset == 0) {
                    for (UINT j = 0; j < i; j++) {
                        free((void*)file_types[j].pszName);
                    }

                    free(filter);
                    free(file_types);
                    destroy_lock(lock);
                    IFileSaveDialog_Release(dialog);
                    free(fdi);
                    return CpdFileDialogResult_OutOfMemory;
                }

                offset += add_offset;
            }

            filter[offset] = 0;
            file_types[i].pszName = filter;
            file_types[i].pszSpec = &filter[pattern_offset];
        }

        if (add_wildcard) {
            file_types[file_type_count - 1] = (COMDLG_FILTERSPEC){
                .pszName = L"All files",
                .pszSpec = L"*"
            };
        }

        IFileSaveDialog_SetFileTypes(dialog, file_type_count, file_types);

        for (UINT i = 0; i < filter_count; i++) {
            free((void*)file_types[i].pszName);
        }

        free(file_types);
    }

    fdi->thread = create_thread(dialog_thread, fdi);

    if (fdi->thread == 0) {
        destroy_lock(lock);
        IFileSaveDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    *out_handle = (CpdFileDialogHandle)fdi;

    return CpdFileDialogResult_Success;
}

CpdFileDialogResult request_directory_dialog(
    CpdWindow parent, const char* title,
    CpdFileDialogHandle* out_handle
) {
    IFileOpenDialog* dialog = 0;
    HRESULT result = CoCreateInstance(&CLSID_FileOpenDialog, 0, CLSCTX_ALL, &IID_IFileOpenDialog, (LPVOID*)&dialog);

    if (FAILED(result)) {
        return CpdFileDialogResult_Unsupported;
    }

    if (out_handle == 0) {
        IFileOpenDialog_Release(dialog);
        return CpdFileDialogResult_InvalidCall;
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        IFileOpenDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        destroy_lock(lock);
        IFileOpenDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;
    fdi->dialog = (IFileDialog*)dialog;
    fdi->hWnd = (HWND)parent;

    if (title != 0) {
        int size = MultiByteToWideChar(CP_UTF8, 0, title, -1, 0, 0);
        if (size == 0) {
            destroy_lock(lock);
            IFileOpenDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_InvalidCall;
        }

        WCHAR* wtitle = (WCHAR*)malloc(size * sizeof(WCHAR));
        if (wtitle == 0) {
            destroy_lock(lock);
            IFileOpenDialog_Release(dialog);
            free(fdi);
            return CpdFileDialogResult_OutOfMemory;
        }

        MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, size);

        IFileOpenDialog_SetTitle(dialog, wtitle);
        free(wtitle);
    }

    FILEOPENDIALOGOPTIONS options;
    IFileOpenDialog_GetOptions(dialog, &options);

    options |= FOS_PICKFOLDERS;
    IFileOpenDialog_SetOptions(dialog, options);

    fdi->thread = create_thread(dialog_thread, fdi);

    if (fdi->thread == 0) {
        destroy_lock(lock);
        IFileOpenDialog_Release(dialog);
        free(fdi);
        return CpdFileDialogResult_InternalError;
    }

    *out_handle = (CpdFileDialogHandle)fdi;

    return CpdFileDialogResult_Success;
}

CpdFileResult open_file_from_dialog_handle(
    CpdFileDialogHandle handle, const char* filename,
    CpdFileOpening opening,
    CpdFileMode mode, CpdFileSharing sharing,
    CpdFile* file
) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    if (fdi->result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        return CpdFileResult_InvalidCall;
    }

    if (FAILED(fdi->result)) {
        return CpdFileResult_InternalError;
    }

    bool result_set = false;
    CpdFileResult file_result;

    int wide_filename_len = 0;
    WCHAR* wide_filename = 0;
    
    if (filename != 0) {
        wide_filename_len = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
        wide_filename = (WCHAR*)malloc(wide_filename_len);

        if (wide_filename == 0) {
            return CpdFileResult_OutOfMemory;
        }

        MultiByteToWideChar(CP_UTF8, 0, filename, -1, wide_filename, wide_filename_len);
    }

    IShellItem* shell_item = 0;
    HRESULT result = IFileDialog_GetResult(fdi->dialog, &shell_item);

    // Only one path selected
    if (SUCCEEDED(result)) {
        WCHAR* wide_path;
        result = IShellItem_GetDisplayName(shell_item, SIGDN_FILESYSPATH, &wide_path);
        IShellItem_Release(shell_item);

        if (FAILED(result)) {
            free(wide_filename);
            return CpdFileResult_OutOfMemory;
        }

        if (filename == 0 || wcscmp(wide_path, wide_filename) == 0) {
            result_set = true;
            file_result = open_file_wide(wide_path, opening, mode, sharing, file);
        }

        CoTaskMemFree(wide_path);
    }
    else {
        IFileOpenDialog* open_dialog = 0;
        result = IFileDialog_QueryInterface(fdi->dialog, &IID_IFileOpenDialog, &open_dialog);

        // Two or more paths selected
        IShellItemArray* shell_arr;
        IFileOpenDialog_GetResults(open_dialog, &shell_arr);

        IEnumShellItems* enumerator;
        IShellItemArray_EnumItems(shell_arr, &enumerator);

        while (!result_set && IEnumShellItems_Next(enumerator, 1, &shell_item, 0) == S_OK) {
            WCHAR* wide_path;
            result = IShellItem_GetDisplayName(shell_item, SIGDN_FILESYSPATH, &wide_path);
            IShellItem_Release(shell_item);

            if (FAILED(wide_path)) {
                IEnumShellItems_Release(enumerator);
                IShellItemArray_Release(shell_arr);
                IFileOpenDialog_Release(open_dialog);
                free(wide_filename);

                return CpdFileResult_OutOfMemory;
            }

            if (filename == 0 || wcscmp(wide_path, wide_filename) == 0) {
                result_set = true;
                file_result = open_file_wide(wide_path, opening, mode, sharing, file);
            }

            CoTaskMemFree(wide_path);
        }

        IEnumShellItems_Release(enumerator);
        IShellItemArray_Release(shell_arr);
        IFileOpenDialog_Release(open_dialog);
    }

    free(wide_filename);

    if (result_set) {
        return file_result;
    }

    return CpdFileResult_InvalidPath;
}

bool wait_for_file_dialog(CpdFileDialogHandle handle, uint64_t timeout, char*** paths, uint32_t* count) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    if (timeout == 0) {
        bool result = try_enter_lock(fdi->lock);
        if (result) {
            leave_lock(fdi->lock);
        }

        return result;
    }

    uint64_t timeout_timestamp = timeout == UINT64_MAX ? UINT64_MAX : get_clock_usec() + timeout;

    while (!try_enter_lock(fdi->lock)) {
        if (timeout != UINT64_MAX && timeout_timestamp < get_clock_usec()) {
            return false;
        }

        Sleep(WAIT_SLEEP_TIME_MSEC);
    }

    leave_lock(fdi->lock);

    if (fdi->result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        *count = UINT32_MAX;
        return false;
    }

    if (FAILED(fdi->result)) {
        *count = fdi->result;
        return false;
    }

    if (paths == 0 || count == 0) {
        return true;
    }

    IShellItem* shell_item;
    HRESULT result = IFileDialog_GetResult(fdi->dialog, &shell_item);

    // Only one path selected
    if (SUCCEEDED(result)) {
        WCHAR* wide_path;
        result = IShellItem_GetDisplayName(shell_item, SIGDN_FILESYSPATH, &wide_path);
        IShellItem_Release(shell_item);

        int c_len = WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, 0, 0, 0, 0);
        if (c_len == 0) {
            *count = HRESULT_FROM_WIN32(GetLastError());
            return false;
        }

        char** arr = (char**)malloc(sizeof(char*));
        char* item = (char*)malloc(c_len);
        if (arr == 0 || item == 0) {
            free(arr);
            free(item);
            *count = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
            return false;
        }

        *arr = item;
        WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, item, c_len, 0, 0);

        CoTaskMemFree(wide_path);

        *count = 1;
        *paths = arr;
    }
    else {
        IFileOpenDialog* open_dialog = 0;
        HRESULT result = IFileDialog_QueryInterface(fdi->dialog, &IID_IFileOpenDialog, &open_dialog);

        // Two or more paths selected
        IShellItemArray* shell_arr;
        IFileOpenDialog_GetResults(open_dialog, &shell_arr);

        DWORD arr_count;
        IEnumShellItems* enumerator;
        IShellItemArray_GetCount(shell_arr, &arr_count);

        char** arr = (char**)malloc(arr_count * sizeof(char*));
        if (arr == 0) {
            IShellItemArray_Release(shell_arr);
            IFileOpenDialog_Release(open_dialog);
            return false;
        }

        IShellItemArray_EnumItems(shell_arr, &enumerator);

        for (DWORD i = 0; i < arr_count && IEnumShellItems_Next(enumerator, 1, &shell_item, 0) == S_OK; i++) {
            WCHAR* wide_path;
            IShellItem_GetDisplayName(shell_item, SIGDN_FILESYSPATH, &wide_path);
            IShellItem_Release(shell_item);

            int c_len = WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, 0, 0, 0, 0);
            if (c_len == 0) {
                for (DWORD j = 0; j < i; j++) {
                    free(arr[j]);
                }

                *count = HRESULT_FROM_WIN32(GetLastError());
                CoTaskMemFree(wide_path);
                free(arr);
                IEnumShellItems_Release(enumerator);
                IShellItemArray_Release(shell_arr);
                IFileOpenDialog_Release(open_dialog);
                return false;
            }

            char* path = (char*)malloc(c_len);
            if (path == 0) {
                for (DWORD j = 0; j < i; j++) {
                    free(arr[j]);
                }

                *count = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
                CoTaskMemFree(wide_path);
                free(arr);
                IEnumShellItems_Release(enumerator);
                IShellItemArray_Release(shell_arr);
                IFileOpenDialog_Release(open_dialog);
                return false;
            }

            arr[i] = path;
            WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, path, c_len, 0, 0);

            CoTaskMemFree(wide_path);
        }

        IEnumShellItems_Release(enumerator);
        IShellItemArray_Release(shell_arr);
        IFileOpenDialog_Release(open_dialog);

        *paths = arr;
        *count = arr_count;
    }

    return true;
}

void cancel_file_dialog(CpdFileDialogHandle handle) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    IFileDialog_Close(fdi->dialog, HRESULT_FROM_WIN32(ERROR_CANCELLED));
}

void destroy_file_dialog_handle(CpdFileDialogHandle handle) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    IFileDialog_Release(fdi->dialog);
    destroy_lock(fdi->lock);

    free(fdi);
}
