#define CINTERFACE
#define COBJMACROS

#include <Cpeed/platform/file.h>
#include <Cpeed/platform/lock.h>

#include <assert.h>
#include <malloc.h>
#include <roapi.h>
#include <string.h>

#include <Windows.Storage.h>
#include <Windows.Storage.Streams.h>
#include <pathcch.h>

#include "uwpmain.h"
#include "winrt_helpers.h"

#define FILE_EVENT_ID_START L" "
#define FILE_EVENT_ID_LENGTH 95

typedef struct CpdWindowsFile {
    CpdFileMode mode;
    CpdFileSharing sharing;

    char* path;
    HANDLE sync_obj;
    __x_ABI_CWindows_CStorage_CIStorageFile* handle;
    __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream* stream;
} CpdWindowsFile;

EXTERN_C const IID IID___FIAsyncOperationCompletedHandler_1_UINT32 = { 0x9343B6E7, 0xE3D2, 0x5E4A, 0xAB, 0x2D, 0x2B, 0xCE, 0x49, 0x19, 0xA6, 0xA4 };
EXTERN_C const IID IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream = { 0x398C4183, 0x793D, 0x5B00, 0x81, 0x9B, 0x4A, 0xEF, 0x92, 0x48, 0x5E, 0x94 };
EXTERN_C const IID IID___x_Windows_CStorage_CStreams_CIDataReaderFactory = { 0xD7527847, 0x57DA, 0x4E15, 0x91, 0x4C, 0x06, 0x80, 0x66, 0x99, 0xA0, 0x98 };
EXTERN_C const IID IID___x_Windows_CStorage_CStreams_CIDataWriterFactory = { 0x338C67C2, 0x8B84, 0x4C2B, 0x9C, 0x50, 0x7B, 0x87, 0x67, 0x84, 0x7A, 0x1F };
EXTERN_C const IID IID___x_Windows_CStorage_CIStorageFile2;
EXTERN_C const IID IID___x_Windows_CStorage_CIStorageFolderStatics;
EXTERN_C const IID IID___x_Windows_CStorage_CIStorageItem;

static uint32_t open_file_event_counter = 0;

static void destroy_file(CpdWindowsFile* win_file) {
    if (win_file->stream != 0) {
        __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_Release(win_file->stream);
    }

    if (win_file->handle != 0) {
        __x_ABI_CWindows_CStorage_CIStorageFile_Release(win_file->handle);
    }

    if (win_file->sync_obj != 0) {
        CloseHandle(win_file->sync_obj);
    }

    if (win_file->path != 0) {
        free(win_file->path);
    }

    free(win_file);
}

static HRESULT sync_handler(void* context, void* async_info, AsyncStatus async_status) {
    if (async_status != Started) {
        HANDLE event = (HANDLE)context;

        SetEvent(event);
    }

    return S_OK;
}

static CpdFileResult get_storage_folder_from_path(WCHAR* path, HANDLE sync_obj, __x_ABI_CWindows_CStorage_CIStorageFolder** folder) {
    HSTRING str;
    HRESULT result = WindowsCreateString(RuntimeClass_Windows_Storage_StorageFolder, (UINT32)wcslen(RuntimeClass_Windows_Storage_StorageFolder), &str);
    if (FAILED(result) || str == 0) {
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CIStorageFolderStatics* folder_statics = 0;
    result = RoGetActivationFactory(str, &IID___x_Windows_CStorage_CIStorageFolderStatics, &folder_statics);
    WindowsDeleteString(str);

    if (FAILED(result)) {
        return CpdFileResult_InternalError;
    }

    result = WindowsCreateString(path, (UINT32)wcslen(path), &str);
    if (FAILED(result)) {
        __x_ABI_CWindows_CStorage_CIStorageFolderStatics_Release(folder_statics);
        return CpdFileResult_OutOfMemory;
    }

    __FIAsyncOperation_1_Windows__CStorage__CStorageFolder* folder_op;
    result = __x_ABI_CWindows_CStorage_CIStorageFolderStatics_GetFolderFromPathAsync(folder_statics, str, &folder_op);
    __x_ABI_CWindows_CStorage_CIStorageFolderStatics_Release(folder_statics);
    WindowsDeleteString(str);

    if (FAILED(result)) {
        return CpdFileResult_InternalError;
    }

    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder* folder_handler = (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder, sync_handler, sync_obj);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder_AddRef(folder_handler);

    result = __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_put_Completed(folder_op, folder_handler);
    
    WaitForSingleObject(sync_obj, INFINITE);

    result = __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_GetResults(folder_op, folder);

    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFolder_Release(folder_handler);
    __FIAsyncOperation_1_Windows__CStorage__CStorageFolder_Release(folder_op);

    switch (result) {
        case S_OK:
            return CpdFileResult_Success;

        case E_ACCESSDENIED:
            return CpdFileResult_NoPermission;

        default:
            return CpdFileResult_InternalError;
    }
}

CpdFileResult open_file_from_storage_file(__x_ABI_CWindows_CStorage_CIStorageFile* storage_file, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file) {
    if (opening != CpdFileOpening_Open && opening != CpdFileOpening_Create) {
        return CpdFileResult_InvalidCall;
    }

    if (storage_file == 0) {
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

    CpdWindowsFile* win_file = (CpdWindowsFile*)malloc(sizeof(CpdWindowsFile));
    if (win_file == 0) {
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CIStorageFile_AddRef(storage_file);

    win_file->path = 0;
    win_file->sync_obj = 0;
    win_file->handle = storage_file;
    win_file->stream = 0;

    __x_ABI_CWindows_CStorage_CIStorageItem* storage_item = 0;
    __x_ABI_CWindows_CStorage_CIStorageItem_QueryInterface(storage_file, &IID___x_Windows_CStorage_CIStorageItem, &storage_item);

    HSTRING str;
    HRESULT result = __x_ABI_CWindows_CStorage_CIStorageItem_get_Path(storage_item, &str);
    if (FAILED(result)) {
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    UINT32 wide_len;
    LPCWCH wide_path = WindowsGetStringRawBuffer(str, &wide_len);

    {
        int req_len = WideCharToMultiByte(CP_UTF8, 0, wide_path, wide_len, NULL, 0, NULL, NULL);

        win_file->path = (char*)malloc((size_t)req_len + 1);
        if (win_file->path == 0) {
            WindowsDeleteString(str);
            destroy_file(win_file);
            return CpdFileResult_OutOfMemory;
        }

        WideCharToMultiByte(CP_UTF8, 0, wide_path, wide_len, win_file->path, req_len, NULL, NULL);
        win_file->path[req_len] = 0;
    }

    WindowsDeleteString(str);

    WCHAR event_name[] = L"LocalCpeedOpenFileEvent" FILE_EVENT_ID_START FILE_EVENT_ID_START;
    event_name[(sizeof(event_name) / sizeof(*event_name)) - 3] += open_file_event_counter % (FILE_EVENT_ID_LENGTH * FILE_EVENT_ID_LENGTH);
    event_name[(sizeof(event_name) / sizeof(*event_name)) - 2] += open_file_event_counter % FILE_EVENT_ID_LENGTH;

    open_file_event_counter++;

    HANDLE sync_obj = CreateEventW(0, FALSE, FALSE, event_name);
    if (sync_obj == 0) {
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    win_file->sync_obj = sync_obj;

    __x_ABI_CWindows_CStorage_CIStorageFile2* storage_file2 = 0;
    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream* stream_op;

    __x_ABI_CWindows_CStorage_CIStorageFile_QueryInterface(win_file->handle, &IID___x_Windows_CStorage_CIStorageFile2, &storage_file2);
    result = __x_ABI_CWindows_CStorage_CIStorageFile2_OpenWithOptionsAsync(storage_file2, file_mode, sharing_flags, &stream_op);
    __x_ABI_CWindows_CStorage_CIStorageFile2_Release(storage_file2);

    if (FAILED(result)) {
        destroy_file(win_file);

        switch (result) {
            case __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
                return CpdFileResult_NoPermission;

            default:
                return CpdFileResult_InvalidCall;
        }
    }

    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream* stream_handler = (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream, sync_handler, sync_obj);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream_AddRef(stream_handler);

    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream_put_Completed(stream_op, stream_handler);

    WaitForSingleObject(sync_obj, INFINITE);

    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream_GetResults(stream_op, &win_file->stream);
    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream_Release(stream_op);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream_Release(stream_handler);

    win_file->mode = mode;
    win_file->sharing = sharing;
    *file = (CpdFile)win_file;

    return CpdFileResult_Success;
}

CpdFileResult file_exists(const char* path) {
    DWORD attr = GetFileAttributesA(path);

    return attr != INVALID_FILE_ATTRIBUTES ? CpdFileResult_Success : CpdFileResult_InvalidPath;
}

CpdFileResult open_file(const char* path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file) {
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

    CpdWindowsFile* win_file = (CpdWindowsFile*)malloc(sizeof(CpdWindowsFile));
    if (win_file == 0) {
        return CpdFileResult_OutOfMemory;
    }

    win_file->path = 0;
    win_file->sync_obj = 0;
    win_file->handle = 0;
    win_file->stream = 0;

    size_t path_size = strlen(path);
    win_file->path = (char*)malloc((path_size + 1) * sizeof(char));
    if (win_file->path == 0) {
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    strcpy_s(win_file->path, path_size + 1, path);

    WCHAR* wide_path = wide_string_from_utf8(path, 0);
    if (wide_path == 0) {
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    DWORD full_path_length = GetFullPathNameW(wide_path, 0, 0, 0);
    WCHAR* wide_full_path_filename;
    WCHAR* wide_full_path = (WCHAR*)malloc(full_path_length * sizeof(WCHAR));
    if (wide_full_path == 0) {
        free(wide_path);
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    GetFullPathNameW(wide_path, full_path_length, wide_full_path, &wide_full_path_filename);
    free(wide_path);

    HSTRING filename;
    HRESULT result = WindowsCreateString(wide_full_path_filename, (UINT32)wcslen(wide_full_path_filename), &filename);
    if (FAILED(result)) {
        free(wide_full_path);
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    PathCchRemoveFileSpec(wide_full_path, full_path_length);

    WCHAR event_name[] = L"LocalCpeedOpenFileEvent" FILE_EVENT_ID_START FILE_EVENT_ID_START;
    event_name[(sizeof(event_name) / sizeof(*event_name)) - 3] += open_file_event_counter % (FILE_EVENT_ID_LENGTH * FILE_EVENT_ID_LENGTH);
    event_name[(sizeof(event_name) / sizeof(*event_name)) - 2] += open_file_event_counter % FILE_EVENT_ID_LENGTH;

    open_file_event_counter++;

    HANDLE sync_obj = CreateEventW(0, FALSE, FALSE, event_name);
    if (sync_obj == 0) {
        WindowsDeleteString(filename);
        free(wide_full_path);
        destroy_file(win_file);
        return CpdFileResult_OutOfMemory;
    }

    win_file->sync_obj = sync_obj;

    __x_ABI_CWindows_CStorage_CIStorageFolder* folder = 0;
    CpdFileResult cpeed_result = get_storage_folder_from_path(wide_full_path, sync_obj, &folder);
    free(wide_full_path);

    if (cpeed_result != CpdFileResult_Success) {
        WindowsDeleteString(filename);
        destroy_file(win_file);
        return cpeed_result;
    }

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile* file_op;
    if (opening == CpdFileOpening_Open) {
        result = __x_ABI_CWindows_CStorage_CIStorageFolder_GetFileAsync(folder, filename, &file_op);
    }
    else {
        result = __x_ABI_CWindows_CStorage_CIStorageFolder_CreateFileAsync(folder, filename, CreationCollisionOption_FailIfExists, &file_op);
    }

    __x_ABI_CWindows_CStorage_CIStorageFolder_Release(folder);
    WindowsDeleteString(filename);

    if (FAILED(result)) {
        destroy_file(win_file);

        switch (result) {
            case __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
                return CpdFileResult_InvalidPath;

            case __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
                return CpdFileResult_NoPermission;

            default:
                return CpdFileResult_InvalidCall;
        }
    }

    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile* file_handler = (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile, sync_handler, sync_obj);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile_AddRef(file_handler);

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile_put_Completed(file_op, file_handler);

    WaitForSingleObject(sync_obj, INFINITE);

    __FIAsyncOperation_1_Windows__CStorage__CStorageFile_GetResults(file_op, &win_file->handle);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile_Release(file_handler);
    __FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(file_op);

    __x_ABI_CWindows_CStorage_CIStorageFile2* storage_file2 = 0;
    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream* stream_op;

    __x_ABI_CWindows_CStorage_CIStorageFile_QueryInterface(win_file->handle, &IID___x_Windows_CStorage_CIStorageFile2, &storage_file2);
    result = __x_ABI_CWindows_CStorage_CIStorageFile2_OpenWithOptionsAsync(storage_file2, file_mode, sharing_flags, &stream_op);
    __x_ABI_CWindows_CStorage_CIStorageFile2_Release(storage_file2);

    if (FAILED(result)) {
        destroy_file(win_file);

        switch (result) {
            case __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
                return CpdFileResult_NoPermission;

            default:
                return CpdFileResult_InvalidCall;
        }
    }

    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream* stream_handler = (__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream, sync_handler, sync_obj);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream_AddRef(stream_handler);

    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream_put_Completed(stream_op, stream_handler);

    WaitForSingleObject(sync_obj, INFINITE);

    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream_GetResults(stream_op, &win_file->stream);
    __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream_Release(stream_op);
    __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIRandomAccessStream_Release(stream_handler);

    win_file->mode = mode;
    win_file->sharing = sharing;
    *file = (CpdFile)win_file;

    return CpdFileResult_Success;
}

const char* file_path(CpdFile file) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    return win_file->path;
}

CpdFileMode file_mode(CpdFile file) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    return win_file->mode;
}

CpdFileSharing file_sharing(CpdFile file) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    return win_file->sharing;
}

int64_t file_pos(CpdFile file) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    UINT64 pos;
    HRESULT result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_get_Position(win_file->stream, &pos);

    if (FAILED(result)) {
        return -1;
    }

    if (pos > INT64_MAX) {
        return (int64_t)INT64_MAX;
    }

    return (int64_t)pos;
}

bool set_file_pos(CpdFile file, CpdFileOffsetType type, int64_t value) {
    if ((uint32_t)type > CpdFileOffsetType_End) {
        return false;
    }

    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    int64_t pos;
    HRESULT result;
    if (type == CpdFileOffsetType_Begin) {
        pos = value;
    }
    else if (type == CpdFileOffsetType_Current) {
        pos = file_pos(file) + value;
    }
    else {
        UINT64 size;
        result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_get_Size(win_file->stream, &size);

        if (FAILED(result)) {
            return false;
        }

        pos = (int64_t)size;
    }

    if (pos + value > INT64_MAX) {
        return false;
    }

    result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_Seek(win_file->stream, pos + value);

    return SUCCEEDED(result);
}

CpdFileResult read_file(CpdFile file, void* buffer, uint32_t* length) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    UINT64 pos;
    __x_ABI_CWindows_CStorage_CStreams_CIInputStream* input_stream;

    HRESULT result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_get_Position(win_file->stream, &pos);
    if (FAILED(result)) {
        return CpdFileResult_InvalidCall;
    }

    result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_GetInputStreamAt(win_file->stream, pos, &input_stream);
    if (FAILED(result)) {
        return CpdFileResult_InvalidCall;
    }

    HSTRING str;
    result = WindowsCreateString(RuntimeClass_Windows_Storage_Streams_DataReader, (UINT32)wcslen(RuntimeClass_Windows_Storage_Streams_DataReader), &str);
    if (FAILED(result) || str == 0) {
        __x_ABI_CWindows_CStorage_CStreams_CIInputStream_Release(input_stream);
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CStreams_CIDataReaderFactory* drf = 0;
    result = RoGetActivationFactory(str, &IID___x_Windows_CStorage_CStreams_CIDataReaderFactory, &drf);
    WindowsDeleteString(str);

    if (FAILED(result) || drf == 0) {
        __x_ABI_CWindows_CStorage_CStreams_CIInputStream_Release(input_stream);
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CStreams_CIDataReader* reader;
    result = __x_ABI_CWindows_CStorage_CStreams_CIDataReaderFactory_CreateDataReader(drf, input_stream, &reader);
    __x_ABI_CWindows_CStorage_CStreams_CIDataReaderFactory_Release(drf);
    __x_ABI_CWindows_CStorage_CStreams_CIInputStream_Release(input_stream);

    if (FAILED(result)) {
        return CpdFileResult_OutOfMemory;
    }

    __FIAsyncOperation_1_UINT32* load_op;
    result = __x_ABI_CWindows_CStorage_CStreams_CIDataReader_LoadAsync(reader, *length, &load_op);

    if (FAILED(result)) {
        __x_ABI_CWindows_CStorage_CStreams_CIDataReader_Release(reader);
        return CpdFileResult_OutOfMemory;
    }

    __FIAsyncOperationCompletedHandler_1_UINT32* load_handler = (__FIAsyncOperationCompletedHandler_1_UINT32*)create_wrapper_for_IAsyncOperationCompletedHandler(&IID___FIAsyncOperationCompletedHandler_1_UINT32, sync_handler, win_file->sync_obj);
    __FIAsyncOperationCompletedHandler_1_UINT32_AddRef(load_handler);
    
    __FIAsyncOperation_1_UINT32_put_Completed(load_op, load_handler);

    WaitForSingleObject(win_file->sync_obj, INFINITE);

    result = __FIAsyncOperation_1_UINT32_GetResults(load_op, length);
    __FIAsyncOperation_1_UINT32_Release(load_op);
    __FIAsyncOperationCompletedHandler_1_UINT32_Release(load_handler);

    if (FAILED(result)) {
        __x_ABI_CWindows_CStorage_CStreams_CIDataReader_Release(reader);
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CStreams_CIDataReader_ReadBytes(reader, *length, buffer);
    __x_ABI_CWindows_CStorage_CStreams_CIDataReader_Release(reader);

    return CpdFileResult_Success;
}

CpdFileResult write_file(CpdFile file, void* buffer, uint32_t* length) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    UINT64 pos;
    __x_ABI_CWindows_CStorage_CStreams_CIOutputStream* output_stream;

    HRESULT result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_get_Position(win_file->stream, &pos);
    if (FAILED(result)) {
        return CpdFileResult_InvalidCall;
    }

    result = __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream_GetOutputStreamAt(win_file->stream, pos, &output_stream);
    if (FAILED(result)) {
        return CpdFileResult_InvalidCall;
    }

    HSTRING str;
    result = WindowsCreateString(RuntimeClass_Windows_Storage_Streams_DataWriter, (UINT32)wcslen(RuntimeClass_Windows_Storage_Streams_DataWriter), &str);
    if (FAILED(result) || str == 0) {
        __x_ABI_CWindows_CStorage_CStreams_CIOutputStream_Release(output_stream);
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory* dwf = 0;
    result = RoGetActivationFactory(str, &IID___x_Windows_CStorage_CStreams_CIDataWriterFactory, &dwf);
    WindowsDeleteString(str);

    if (FAILED(result) || dwf == 0) {
        __x_ABI_CWindows_CStorage_CStreams_CIOutputStream_Release(output_stream);
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CStreams_CIDataWriter* writer;
    result = __x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory_CreateDataWriter(dwf, output_stream, &writer);
    __x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory_Release(dwf);
    __x_ABI_CWindows_CStorage_CStreams_CIOutputStream_Release(output_stream);

    if (FAILED(result)) {
        return CpdFileResult_OutOfMemory;
    }

    __x_ABI_CWindows_CStorage_CStreams_CIDataWriter_WriteBytes(writer, *length, buffer);
    __x_ABI_CWindows_CStorage_CStreams_CIDataWriter_Release(writer);

    return CpdFileResult_Success;
}

void close_file(CpdFile file) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    destroy_file(win_file);
}
