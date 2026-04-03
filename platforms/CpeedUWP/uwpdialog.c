#define CINTERFACE
#define COBJMACROS

#ifdef WINAPI_FAMILY
#undef WINAPI_FAMILY
#endif

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP

#include <Cpeed/platform/dialog.h>
#include <Cpeed/platform/lock.h>
#include <Cpeed/platform/logging.h>
#include <Cpeed/platform.h>

#include <malloc.h>

#include "uwpfile.h"
#include "uwpmain.h"
#include "winrt_helpers.h"

#include <CoreWindow.h>
#include <inspectable.h>
#include <roapi.h>
#include <shobjidl.h>
#include <winstring.h>
#include <Windows.Storage.Pickers.h>
#include <Windows.UI.Core.h>

#define WAIT_SLEEP_TIME_MSEC 100

EXTERN_C const IID IID_ICoreWindowInterop = { 0x45D64A29, 0xA63E, 0x4CB6, 0xB4, 0x98, 0x57, 0x81, 0xD2, 0x98, 0xCB, 0x4F };
EXTERN_C const IID IID___x_Windows_CStorage_CPickers_CIFileOpenPicker;
EXTERN_C const IID IID___x_Windows_CStorage_CPickers_CIFileSavePicker;
EXTERN_C const IID IID___x_Windows_CStorage_CPickers_CIFolderPicker;
EXTERN_C const IID IID___x_Windows_CStorage_CIStorageItem;

static const WCHAR file_pattern_separator = L';';

typedef struct CpdFileDialogInternal {
    CpdLock lock;
    IInspectable* async_operation;
    IUnknown* async_handler;
    HRESULT result;
} CpdFileDialogInternal;

bool file_dialog_supported() {
    return true;
}

static HRESULT dialog_handler(void* context, IInspectable* async_info, AsyncStatus async_status) {
    if (async_status == Started) {
        return S_OK;
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)context;

    if (async_status == Completed) {
        fdi->result = S_OK;
    }
    else if (async_status == Canceled) {
        fdi->result = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    else {
        fdi->result = E_FAIL;
    }

    leave_lock(fdi->lock);

    return S_OK;
}

CpdFileDialogResult request_open_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    bool multiple, bool add_wildcard,
    CpdFileDialogHandle* out_handle
) {
    if (out_handle == 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    if (filters == 0 && filter_count > 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    if (filter_count == 0 && !add_wildcard) {
        return CpdFileDialogResult_InvalidCall;
    }

    HSTRING class_id;
    HRESULT result = WindowsCreateString(
        RuntimeClass_Windows_Storage_Pickers_FileOpenPicker,
        (UINT32)wcslen(RuntimeClass_Windows_Storage_Pickers_FileOpenPicker),
        &class_id);

    if (FAILED(result) || class_id == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    IInspectable* inspectable = 0;
    result = RoActivateInstance(class_id, &inspectable);

    WindowsDeleteString(class_id);

    if (FAILED(result)) {
        return CpdFileDialogResult_Unsupported;
    }

    __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker* open_picker = 0;
    result = IInspectable_QueryInterface(inspectable, &IID___x_Windows_CStorage_CPickers_CIFileOpenPicker, &open_picker);
    IInspectable_Release(inspectable);

    if (FAILED(result)) {
        IInspectable_Release(inspectable);
        return CpdFileDialogResult_InternalError;
    }

    IUnknown* core_window = (IUnknown*)(parent != 0 ? ((CpdUWPWindow*)parent)->core_window : g_main_core_window);

    ICoreWindowInterop* interop = 0;
    result = IUnknown_QueryInterface(core_window, &IID_ICoreWindowInterop, &interop);
    if (SUCCEEDED(result)) {
        HWND hWnd;
        result = ICoreWindowInterop_get_WindowHandle(interop, &hWnd);
        
        if (SUCCEEDED(result)) {
            IInitializeWithWindow* iww = 0;
            result = __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_QueryInterface(open_picker, &IID_IInitializeWithWindow, &iww);

            if (SUCCEEDED(result) && iww != 0) {
                IInitializeWithWindow_Initialize(iww, hWnd);
                IInitializeWithWindow_Release(iww);
            }
        }

        ICoreWindowInterop_Release(interop);
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_Release(open_picker);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        destroy_lock(lock);
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_Release(open_picker);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;

    __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_put_ViewMode(open_picker, PickerViewMode_List);

    __FIVector_1_HSTRING* array = 0;
    result = __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_get_FileTypeFilter(open_picker, &array);

    for (size_t i = 0; i < filter_count; i++) {
        CpdFileDialogSelector* selectors = filters[i].selectors;
        size_t selector_count = filters[i].selector_count;

        for (size_t j = 0; j < selector_count; j++) {
            const char* selector = selectors[j].pattern;
            const char* file_type = 0;

            while (true) {
                const char* next = strstr(selector, "*");
                if (next == 0) {
                    break;
                }

                file_type = next;
                selector = next + 1;
            }

            if (file_type == 0) {
                file_type = selector;
            }

            file_type = strstr(file_type, ".");

            if (file_type == 0) {
                continue;
            }

            size_t file_type_len = strlen(file_type);
            int required_size = MultiByteToWideChar(CP_UTF8, 0, file_type, (int)file_type_len, 0, 0);
            
            WCHAR* wide_file_type = (WCHAR*)malloc((size_t)required_size * sizeof(WCHAR));
            if (wide_file_type == 0) {
                __FIVector_1_HSTRING_Release(array);
                leave_lock(lock);
                destroy_lock(lock);
                free(fdi);
                __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_Release(open_picker);
                return CpdFileDialogResult_OutOfMemory;
            }

            MultiByteToWideChar(CP_UTF8, 0, file_type, (int)file_type_len, wide_file_type, required_size);

            HSTRING str;
            result = WindowsCreateString(wide_file_type, required_size, &str);
            free(wide_file_type);

            if (FAILED(result)) {
                __FIVector_1_HSTRING_Release(array);
                leave_lock(lock);
                destroy_lock(lock);
                free(fdi);
                __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_Release(open_picker);
                return CpdFileDialogResult_OutOfMemory;
            }

            UINT32 index;
            boolean exists = false;
            __FIVector_1_HSTRING_IndexOf(array, str, &index, &exists);

            if (!exists) {
                __FIVector_1_HSTRING_Append(array, str);
            }

            WindowsDeleteString(str);
        }
    }

    if (add_wildcard) {
        HSTRING wildcard;
        WindowsCreateString(L"*", 1, &wildcard);

        __FIVector_1_HSTRING_Append(array, wildcard);

        WindowsDeleteString(wildcard);
    }

    __FIVector_1_HSTRING_Release(array);

    IInspectable* async_operation = 0;
    IUnknown* handler = 0;

    if (multiple) {
        result = __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_PickMultipleFilesAsync(open_picker, (__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile**)&async_operation);
    }
    else {
        result = __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_PickSingleFileAsync(open_picker, (__FIAsyncOperation_1_Windows__CStorage__CStorageFile**)&async_operation);
    }

    if (FAILED(result)) {
        leave_lock(lock);
        destroy_lock(lock);
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_Release(open_picker);
        return CpdFileDialogResult_InternalError;
    }

    if (multiple) {
        handler = (IUnknown*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1___FIVectorView_1_Windows__CStorage__CStorageFile, dialog_handler, fdi);
        IUnknown_AddRef(handler);

        __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_put_Completed((__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile*)async_operation, (__FIAsyncOperationCompletedHandler_1___FIVectorView_1_Windows__CStorage__CStorageFile*)handler);
    }
    else {
        handler = (IUnknown*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile, dialog_handler, fdi);
        IUnknown_AddRef(handler);

        __FIAsyncOperation_1_Windows__CStorage__CStorageFile_put_Completed((__FIAsyncOperation_1_Windows__CStorage__CStorageFile*)async_operation, (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile*)handler);
    }

    fdi->async_operation = async_operation;
    fdi->async_handler = handler;

    *out_handle = (CpdFileDialogHandle)fdi;

    __x_ABI_CWindows_CStorage_CPickers_CIFileOpenPicker_Release(open_picker);

    return CpdFileDialogResult_Success;
}

CpdFileDialogResult request_save_file_dialog(
    CpdWindow parent, const char* title,
    const CpdFileDialogFilter* filters, size_t filter_count,
    const char* name, const char* folder, bool add_wildcard,
    CpdFileDialogHandle* out_handle
) {
    if (out_handle == 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    if (filter_count == 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    HSTRING class_id = 0;
    HRESULT result = WindowsCreateString(
        RuntimeClass_Windows_Storage_Pickers_FileSavePicker,
        (UINT32)wcslen(RuntimeClass_Windows_Storage_Pickers_FileSavePicker),
        &class_id);

    if (FAILED(result) || class_id == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    IInspectable* inspectable = 0;
    result = RoActivateInstance(class_id, &inspectable);

    WindowsDeleteString(class_id);

    if (FAILED(result)) {
        return CpdFileDialogResult_Unsupported;
    }

    __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker* save_picker = 0;
    result = IInspectable_QueryInterface(inspectable, &IID___x_Windows_CStorage_CPickers_CIFileSavePicker, &save_picker);
    IInspectable_Release(inspectable);

    if (FAILED(result)) {
        IInspectable_Release(inspectable);
        return CpdFileDialogResult_InternalError;
    }

    IUnknown* core_window = (IUnknown*)(parent != 0 ? ((CpdUWPWindow*)parent)->core_window : g_main_core_window);

    ICoreWindowInterop* interop = 0;
    result = IUnknown_QueryInterface(core_window, &IID_ICoreWindowInterop, &interop);
    if (SUCCEEDED(result)) {
        HWND hWnd;
        result = ICoreWindowInterop_get_WindowHandle(interop, &hWnd);

        if (SUCCEEDED(result)) {
            IInitializeWithWindow* iww = 0;
            result = __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_QueryInterface(save_picker, &IID_IInitializeWithWindow, &iww);

            if (SUCCEEDED(result) && iww != 0) {
                IInitializeWithWindow_Initialize(iww, hWnd);
                IInitializeWithWindow_Release(iww);
            }
        }

        ICoreWindowInterop_Release(interop);
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        destroy_lock(lock);
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;

    __FIMap_2_HSTRING___FIVector_1_HSTRING* map = 0;
    __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_get_FileTypeChoices(save_picker, &map);

    for (size_t i = 0; i < filter_count; i++) {
        CpdFileDialogSelector* selectors = filters[i].selectors;
        size_t selector_count = filters[i].selector_count;

        __FIVector_1_HSTRING* extensions = (__FIVector_1_HSTRING*)create_wrapper_for_IVector_HSTRING();
        if (extensions == 0) {
            __FIMap_2_HSTRING___FIVector_1_HSTRING_Release(map);
            leave_lock(lock);
            destroy_lock(lock);
            free(fdi);
            __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
            return CpdFileDialogResult_OutOfMemory;
        }

        __FIVector_1_HSTRING_AddRef(extensions);

        for (size_t j = 0; j < selector_count; j++) {
            const char* selector = selectors[j].pattern;
            const char* file_type = 0;

            while (true) {
                const char* next = strstr(selector, "*");
                if (next == 0) {
                    break;
                }

                file_type = next;
                selector = next + 1;
            }

            if (file_type == 0) {
                file_type = selector;
            }

            file_type = strstr(file_type, ".");

            if (file_type == 0) {
                continue;
            }

            size_t file_type_len = strlen(file_type);
            int required_size = MultiByteToWideChar(CP_UTF8, 0, file_type, (int)file_type_len, 0, 0);

            WCHAR* wide_file_type = (WCHAR*)malloc((size_t)required_size * sizeof(WCHAR));
            if (wide_file_type == 0) {
                __FIVector_1_HSTRING_Release(extensions);
                __FIMap_2_HSTRING___FIVector_1_HSTRING_Release(map);
                leave_lock(lock);
                destroy_lock(lock);
                free(fdi);
                __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
                return CpdFileDialogResult_OutOfMemory;
            }

            MultiByteToWideChar(CP_UTF8, 0, file_type, (int)file_type_len, wide_file_type, required_size);

            HSTRING str;
            result = WindowsCreateString(wide_file_type, required_size, &str);
            free(wide_file_type);

            if (FAILED(result)) {
                __FIVector_1_HSTRING_Release(extensions);
                __FIMap_2_HSTRING___FIVector_1_HSTRING_Release(map);
                leave_lock(lock);
                destroy_lock(lock);
                free(fdi);
                __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
                return CpdFileDialogResult_OutOfMemory;
            }

            UINT32 index;
            boolean exists = false;
            __FIVector_1_HSTRING_IndexOf(extensions, str, &index, &exists);

            if (!exists) {
                __FIVector_1_HSTRING_Append(extensions, str);
            }

            WindowsDeleteString(str);
        }

        UINT32 vector_size = 0;
        __FIVector_1_HSTRING_get_Size(extensions, &vector_size);

        if (vector_size != 0) {
            size_t name_len = strlen(filters[i].name);
            int required_size = MultiByteToWideChar(CP_UTF8, 0, filters[i].name, (int)name_len, 0, 0);

            WCHAR* wide_name = (WCHAR*)malloc((size_t)required_size * sizeof(WCHAR));
            if (wide_name == 0) {
                __FIVector_1_HSTRING_Release(extensions);
                __FIMap_2_HSTRING___FIVector_1_HSTRING_Release(map);
                leave_lock(lock);
                destroy_lock(lock);
                free(fdi);
                __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
                return CpdFileDialogResult_OutOfMemory;
            }
            
            MultiByteToWideChar(CP_UTF8, 0, filters[i].name, (int)name_len, wide_name, required_size);

            HSTRING str;
            result = WindowsCreateString(wide_name, required_size, &str);
            free(wide_name);

            if (SUCCEEDED(result)) {
                boolean success = FALSE;
                __FIMap_2_HSTRING___FIVector_1_HSTRING_Insert(map, str, extensions, &success);
            }

            WindowsDeleteString(str);
        }

        __FIVector_1_HSTRING_Release(extensions);
    }

    __FIMap_2_HSTRING___FIVector_1_HSTRING_Release(map);

    if (name != 0) {
        size_t name_len = strlen(name);
        int required_size = MultiByteToWideChar(CP_UTF8, 0, name, (int)name_len, 0, 0);

        WCHAR* wide_name = (WCHAR*)malloc(name_len * sizeof(WCHAR));
        if (wide_name != 0) {
            MultiByteToWideChar(CP_UTF8, 0, name, (int)name_len, wide_name, (int)name_len);

            HSTRING str;
            if (SUCCEEDED(WindowsCreateString(wide_name, required_size, &str))) {
                __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_put_SuggestedFileName(save_picker, str);

                WindowsDeleteString(str);
            }

            free(wide_name);
        }
    }

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile* async_operation = 0;
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile* handler = 0;

    result = __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_PickSaveFileAsync(save_picker, &async_operation);

    if (FAILED(result) || async_operation == 0) {
        leave_lock(lock);
        destroy_lock(lock);
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);
        return CpdFileDialogResult_InternalError;
    }

    handler = (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile, dialog_handler, fdi);
    IUnknown_AddRef(handler);

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile_put_Completed(async_operation, handler);

    fdi->async_operation = (IInspectable*)async_operation;
    fdi->async_handler = (IUnknown*)handler;

    *out_handle = (CpdFileDialogHandle)fdi;

    __x_ABI_CWindows_CStorage_CPickers_CIFileSavePicker_Release(save_picker);

    return CpdFileDialogResult_Success;
}

CpdFileDialogResult request_directory_dialog(
    CpdWindow parent, const char* title,
    CpdFileDialogHandle* out_handle
) {
    if (out_handle == 0) {
        return CpdFileDialogResult_InvalidCall;
    }

    HSTRING class_id;
    HRESULT result = WindowsCreateString(
        RuntimeClass_Windows_Storage_Pickers_FolderPicker,
        (UINT32)wcslen(RuntimeClass_Windows_Storage_Pickers_FolderPicker),
        &class_id);

    if (FAILED(result) || class_id == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    IInspectable* inspectable = 0;
    result = RoActivateInstance(class_id, &inspectable);

    WindowsDeleteString(class_id);

    if (FAILED(result)) {
        return CpdFileDialogResult_Unsupported;
    }

    __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker* folder_picker = 0;
    result = IInspectable_QueryInterface(inspectable, &IID___x_Windows_CStorage_CPickers_CIFolderPicker, &folder_picker);
    IInspectable_Release(inspectable);

    if (FAILED(result)) {
        IInspectable_Release(inspectable);
        return CpdFileDialogResult_InternalError;
    }

    IUnknown* core_window = (IUnknown*)(parent != 0 ? ((CpdUWPWindow*)parent)->core_window : g_main_core_window);

    ICoreWindowInterop* interop = 0;
    result = IUnknown_QueryInterface(core_window, &IID_ICoreWindowInterop, &interop);
    if (SUCCEEDED(result)) {
        HWND hWnd;
        result = ICoreWindowInterop_get_WindowHandle(interop, &hWnd);

        if (SUCCEEDED(result)) {
            IInitializeWithWindow* iww = 0;
            result = __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_QueryInterface(folder_picker, &IID_IInitializeWithWindow, &iww);

            if (SUCCEEDED(result) && iww != 0) {
                IInitializeWithWindow_Initialize(iww, hWnd);
                IInitializeWithWindow_Release(iww);
            }
        }

        ICoreWindowInterop_Release(interop);
    }

    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)malloc(sizeof(CpdFileDialogInternal));
    if (fdi == 0) {
        return CpdFileDialogResult_OutOfMemory;
    }

    CpdLock lock = create_lock();
    if (lock == 0) {
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_Release(folder_picker);
        return CpdFileDialogResult_InternalError;
    }

    if (!enter_lock(lock)) {
        destroy_lock(lock);
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_Release(folder_picker);
        return CpdFileDialogResult_InternalError;
    }

    fdi->lock = lock;

    __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_put_ViewMode(folder_picker, PickerViewMode_List);

    __FIAsyncOperation_1_Windows__CStorage__CStorageFolder* async_operation = 0;
    result = __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_PickSingleFolderAsync(folder_picker, &async_operation);
    if (FAILED(result)) {
        leave_lock(lock);
        destroy_lock(lock);
        free(fdi);
        __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_Release(folder_picker);
        return CpdFileDialogResult_InternalError;
    }

    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder* handler = (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder, dialog_handler, fdi);

    IUnknown_AddRef(handler);

    result = __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_put_Completed(async_operation, handler);

    fdi->async_operation = (IInspectable*)async_operation;
    fdi->async_handler = (IUnknown*)handler;

    *out_handle = (CpdFileDialogHandle)fdi;

    __x_ABI_CWindows_CStorage_CPickers_CIFolderPicker_Release(folder_picker);

    return CpdFileDialogResult_Success;
}

static int wcscmp_len(const wchar_t* s1, size_t l1, const wchar_t* s2, size_t l2) {
    size_t min_len = l1 < l2 ? l1 : l2;

    for (size_t i = 0; i < min_len; i++) {
        if (*s1 > *s2) {
            return 1;
        }

        if (*s1 < *s2) {
            return -1;
        }

        s1++;
        s2++;
    }


    if (l1 > l2) {
        return 1;
    }

    if (l1 < l2) {
        return -1;
    }

    return 0;
}

CpdFileResult open_file_from_dialog_handle(
    CpdFileDialogHandle handle, const char* filename,
    CpdFileOpening opening,
    CpdFileMode mode, CpdFileSharing sharing,
    CpdFile* file
) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile* file_operation = 0;
    __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile* files_operation = 0;
    __FIAsyncOperation_1_Windows__CStorage__CStorageFolder* folder_operation = 0;

    HRESULT result;
    int cmp;

    bool result_set = false;
    CpdFileResult file_result;

    if (opening != CpdFileOpening_Open && opening != CpdFileOpening_Create) {
        return CpdFileResult_InvalidCall;
    }

    enum __x_ABI_CWindows_CStorage_CFileAccessMode file_mode;
    switch (mode) {
        case CpdFileMode_Read:
            file_mode = FileAccessMode_Read;
            break;

        case CpdFileMode_ReadWrite:
            file_mode = FileAccessMode_ReadWrite;
            break;

        default:
            return CpdFileResult_InvalidCall;
    }

    enum __x_ABI_CWindows_CStorage_CStorageOpenOptions sharing_flags;
    switch (sharing) {
        case CpdFileSharing_Exclusive:
            sharing_flags = StorageOpenOptions_None;
            break;

        case CpdFileSharing_ReadOnly:
            sharing_flags = StorageOpenOptions_AllowOnlyReaders;
            break;

        case CpdFileSharing_ReadWrite:
            sharing_flags = StorageOpenOptions_AllowReadersAndWriters;
            break;

        default:
            return CpdFileResult_InvalidCall;
    }

    UINT wide_filename_len = 0;
    WCHAR* wide_filename = 0;
    if (filename != 0) {
        wide_filename = wide_string_from_utf8(filename, &wide_filename_len);

        if (wide_filename == 0) {
            return CpdFileResult_OutOfMemory;
        }
    }

    if (SUCCEEDED(result = IInspectable_QueryInterface(fdi->async_operation, &IID___FIAsyncOperation_1_Windows__CStorage__CStorageFile, &file_operation))) {
        //
        // Open/Save Single File
        //

        __x_ABI_CWindows_CStorage_CIStorageFile* storage_file = 0;
        __x_ABI_CWindows_CStorage_CIStorageItem* storage_item = 0;
        result = __FIAsyncOperation_1_Windows__CStorage__CStorageFile_GetResults(file_operation, &storage_file);
        if (SUCCEEDED(result)) {
            if (storage_file == 0) {
                __FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(file_operation);
                free(wide_filename);
                return CpdFileResult_InvalidCall;
            }

            if (filename == 0) {
                result_set = true;
                file_result = open_file_from_storage_file(storage_file, opening, mode, sharing, file);
            }
            else {
                __x_ABI_CWindows_CStorage_CIStorageFile_QueryInterface(storage_file, &IID___x_Windows_CStorage_CIStorageItem, &storage_item);

                HSTRING path;
                result = __x_ABI_CWindows_CStorage_CIStorageItem_get_Path(storage_item, &path);

                if (SUCCEEDED(result)) {
                    UINT32 len = 0;
                    LPCWSTR raw_path = WindowsGetStringRawBuffer(path, &len);
                    cmp = wcscmp_len(wide_filename, wide_filename_len, raw_path, len);

                    WindowsDeleteString(path);

                    result_set = true;
                    if (cmp == 0) {
                        file_result = open_file_from_storage_file(storage_file, opening, mode, sharing, file);
                    }
                    else {
                        file_result = CpdFileResult_InvalidPath;
                    }
                }

                __x_ABI_CWindows_CStorage_CIStorageItem_Release(storage_item);
                __x_ABI_CWindows_CStorage_CIStorageFile_Release(storage_file);
            }
        }

        __FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(file_operation);

        if (result_set) {
            return file_result;
        }
    }
    else if (SUCCEEDED(result = IInspectable_QueryInterface(fdi->async_operation, &IID___FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile, &files_operation))) {
        //
        // Open Multiple Files
        //

        __FIVectorView_1_Windows__CStorage__CStorageFile* files = 0;
        result = __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_GetResults(files_operation, &files);
        if (SUCCEEDED(result)) {
            UINT32 size = 0;
            __FIVectorView_1_Windows__CStorage__CStorageFile_get_Size(files, &size);

            if (size == 0) {
                __FIVectorView_1_Windows__CStorage__CStorageFile_Release(files);
                __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_Release(files_operation);
                free(wide_filename);
                return CpdFileResult_InvalidCall;
            }
            
            if (filename == 0) {
                __x_ABI_CWindows_CStorage_CIStorageFile* storage_file = 0;
                __FIVectorView_1_Windows__CStorage__CStorageFile_GetAt(files, 0, &storage_file);

                result_set = true;
                file_result = open_file_from_storage_file(storage_file, opening, mode, sharing, file);

                __x_ABI_CWindows_CStorage_CIStorageFile_Release(storage_file);
            }
            else {
                UINT32 i = 0;
                do {
                    __x_ABI_CWindows_CStorage_CIStorageFile* storage_file = 0;
                    __x_ABI_CWindows_CStorage_CIStorageItem* storage_item = 0;
                    __FIVectorView_1_Windows__CStorage__CStorageFile_GetAt(files, i, &storage_file);
                    __x_ABI_CWindows_CStorage_CIStorageFile_QueryInterface(storage_file, &IID___x_Windows_CStorage_CIStorageItem, &storage_item);

                    HSTRING path;
                    result = __x_ABI_CWindows_CStorage_CIStorageItem_get_Path(storage_item, &path);
                    if (SUCCEEDED(result)) {
                        UINT32 len = 0;
                        LPCWSTR raw_path = WindowsGetStringRawBuffer(path, &len);
                        cmp = wcscmp_len(wide_filename, wide_filename_len, raw_path, len);
                        WindowsDeleteString(path);

                        if (cmp == 0) {
                            result_set = true;
                            file_result = open_file_from_storage_file(storage_file, opening, mode, sharing, file);
                        }
                    }

                    __x_ABI_CWindows_CStorage_CIStorageItem_Release(storage_item);
                    __x_ABI_CWindows_CStorage_CIStorageFile_Release(storage_file);
                } while (++i < size && !result_set);
            }

            __FIVectorView_1_Windows__CStorage__CStorageFile_Release(files);
        }

        __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_Release(files_operation);

        if (result_set) {
            return file_result;
        }
    }
    else if (SUCCEEDED(result = IInspectable_QueryInterface(fdi->async_operation, &IID___FIAsyncOperation_1_Windows__CStorage__CStorageFolder, &folder_operation))) {
        //
        // Folder
        //

        __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_Release(folder_operation);
        free(wide_filename);
        
        return CpdFileResult_InvalidCall;
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

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile* file_operation = 0;
    __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile* files_operation = 0;
    __FIAsyncOperation_1_Windows__CStorage__CStorageFolder* folder_operation = 0;

    HRESULT result;
    bool success = false;

    if (SUCCEEDED(result = IInspectable_QueryInterface(fdi->async_operation, &IID___FIAsyncOperation_1_Windows__CStorage__CStorageFile, &file_operation))) {
        //
        // Open/Save Single File
        //

        __x_ABI_CWindows_CStorage_CIStorageFile* file = 0;
        __x_ABI_CWindows_CStorage_CIStorageItem* item = 0;
        result = __FIAsyncOperation_1_Windows__CStorage__CStorageFile_GetResults(file_operation, &file);
        if (SUCCEEDED(result)) {
            if (file == 0) {
                __FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(file_operation);
                *count = UINT32_MAX;
                return false;
            }

            result = __x_ABI_CWindows_CStorage_CIStorageFile_QueryInterface(file, &IID___x_Windows_CStorage_CIStorageItem, &item);

            if (SUCCEEDED(result)) {
                HSTRING path;
                result = __x_ABI_CWindows_CStorage_CIStorageItem_get_Path(item, &path);

                if (SUCCEEDED(result)) {
                    UINT32 len = 0;
                    LPCWSTR raw_path = WindowsGetStringRawBuffer(path, &len);

                    int required_len = WideCharToMultiByte(CP_UTF8, 0, raw_path, len, 0, 0, 0, 0);
                    char** path_arr = (char**)malloc(sizeof(char*));
                    char* c_path = (char*)malloc(((size_t)required_len + 1) * sizeof(char));

                    if (path_arr == 0 || c_path == 0) {
                        free(path_arr);
                        free(c_path);
                    }
                    else {
                        path_arr[0] = c_path;

                        WideCharToMultiByte(CP_UTF8, 0, raw_path, len, c_path, required_len + 1, 0, 0);
                        c_path[required_len] = 0;

                        *paths = path_arr;
                        *count = 1;
                        success = true;
                    }
                    
                    WindowsDeleteString(path);
                }

                __x_ABI_CWindows_CStorage_CIStorageItem_Release(item);
            }

            __x_ABI_CWindows_CStorage_CIStorageFile_Release(file);
        }

        __FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(file_operation);
    }
    else if (SUCCEEDED(result = IInspectable_QueryInterface(fdi->async_operation, &IID___FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile, &files_operation))) {
        //
        // Open Multiple Files
        //

        __FIVectorView_1_Windows__CStorage__CStorageFile* files = 0;
        result = __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_GetResults(files_operation, &files);
        if (SUCCEEDED(result)) {
            UINT32 size = 0;
            __FIVectorView_1_Windows__CStorage__CStorageFile_get_Size(files, &size);

            if (size == 0) {
                __FIVectorView_1_Windows__CStorage__CStorageFile_Release(files);
                __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_Release(files_operation);
                *count = UINT32_MAX;
                return false;
            }

            char** path_arr = (char**)malloc((size_t)size * sizeof(char*));
            if (path_arr == 0) {
                __FIVectorView_1_Windows__CStorage__CStorageFile_Release(files);
                __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_Release(files_operation);
                return false;
            }

            UINT32 i = 0;
            do {
                __x_ABI_CWindows_CStorage_CIStorageFile* file = 0;
                __x_ABI_CWindows_CStorage_CIStorageItem* item = 0;
                __FIVectorView_1_Windows__CStorage__CStorageFile_GetAt(files, i, &file);
                __x_ABI_CWindows_CStorage_CIStorageFile_QueryInterface(file, &IID___x_Windows_CStorage_CIStorageItem, &item);
                __x_ABI_CWindows_CStorage_CIStorageFile_Release(file);

                HSTRING path;
                result = __x_ABI_CWindows_CStorage_CIStorageItem_get_Path(item, &path);
                if (SUCCEEDED(result)) {
                    UINT32 len = 0;
                    LPCWSTR raw_path = WindowsGetStringRawBuffer(path, &len);

                    int required_len = WideCharToMultiByte(CP_UTF8, 0, raw_path, len, 0, 0, 0, 0);
                    char* c_path = (char*)malloc(((size_t)required_len + 1) * sizeof(char));

                    if (c_path != 0) {
                        path_arr[i] = c_path;

                        WideCharToMultiByte(CP_UTF8, 0, raw_path, len, c_path, required_len + 1, 0, 0);
                        c_path[required_len] = 0;

                        *paths = path_arr;
                        *count = 1;
                        success = true;
                    }
                    else {
                        for (UINT32 j = 0; j < i; j++) {
                            free(path_arr[j]);
                        }

                        free(path_arr);
                        WindowsDeleteString(path);
                        __x_ABI_CWindows_CStorage_CIStorageItem_Release(item);
                        __FIVectorView_1_Windows__CStorage__CStorageFile_Release(files);
                        __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_Release(files_operation);
                        return false;
                    }

                    WindowsDeleteString(path);
                }
                else {
                    path_arr[i] = 0;
                }

                __x_ABI_CWindows_CStorage_CIStorageItem_Release(item);
            } while (++i < size);

            *paths = path_arr;
            *count = size;
            success = true;

            __FIVectorView_1_Windows__CStorage__CStorageFile_Release(files);
        }

        __FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile_Release(files_operation);
    }
    else if (SUCCEEDED(result = IInspectable_QueryInterface(fdi->async_operation, &IID___FIAsyncOperation_1_Windows__CStorage__CStorageFolder, &folder_operation))) {
        //
        // Folder
        //

        __x_ABI_CWindows_CStorage_CIStorageFolder* folder = 0;
        __x_ABI_CWindows_CStorage_CIStorageItem* item = 0;
        result = __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_GetResults(folder_operation, &folder);
        if (SUCCEEDED(result)) {
            if (folder == 0) {
                __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_Release(folder_operation);
                *count = UINT32_MAX;
                return false;
            }

            result = __x_ABI_CWindows_CStorage_CIStorageFolder_QueryInterface(folder, &IID___x_Windows_CStorage_CIStorageItem, &item);

            if (SUCCEEDED(result)) {
                HSTRING path;
                result = __x_ABI_CWindows_CStorage_CIStorageItem_get_Path(item, &path);

                if (SUCCEEDED(result)) {
                    UINT32 len = 0;
                    LPCWSTR raw_path = WindowsGetStringRawBuffer(path, &len);

                    int required_len = WideCharToMultiByte(CP_UTF8, 0, raw_path, len, 0, 0, 0, 0);
                    char** path_arr = (char**)malloc(sizeof(char*));
                    char* c_path = (char*)malloc(((size_t)required_len + 1) * sizeof(char));

                    if (path_arr == 0 || c_path == 0) {
                        free(path_arr);
                        free(c_path);
                    }
                    else {
                        path_arr[0] = c_path;

                        WideCharToMultiByte(CP_UTF8, 0, raw_path, len, c_path, required_len + 1, 0, 0);
                        c_path[required_len] = 0;

                        *paths = path_arr;
                        *count = 1;
                        success = true;
                    }

                    WindowsDeleteString(path);
                }

                __x_ABI_CWindows_CStorage_CIStorageItem_Release(item);
            }

            __x_ABI_CWindows_CStorage_CIStorageFolder_Release(folder);
        }

        __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_Release(folder_operation);
    }

    return success;
}

void cancel_file_dialog(CpdFileDialogHandle handle) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;
    IAsyncInfo* async_info = 0;

    HRESULT result = IInspectable_QueryInterface(fdi->async_operation, &IID_IAsyncInfo, &async_info);

    if (SUCCEEDED(result)) {
        IAsyncInfo_Cancel(async_info);
        IAsyncInfo_Release(async_info);
    }
}

void destroy_file_dialog_handle(CpdFileDialogHandle handle) {
    CpdFileDialogInternal* fdi = (CpdFileDialogInternal*)handle;

    IInspectable_Release(fdi->async_operation);
    IUnknown_Release(fdi->async_handler);
    destroy_lock(fdi->lock);

    free(fdi);
}
