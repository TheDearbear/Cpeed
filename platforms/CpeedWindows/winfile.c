#include <malloc.h>
#include <string.h>

#include "winfile.h"

CpdFileResult file_exists(const char* path) {
    DWORD attr = GetFileAttributesA(path);

    return attr != INVALID_FILE_ATTRIBUTES ? CpdFileResult_Success : CpdFileResult_InvalidPath;
}

CpdFileResult open_file_wide(LPCWSTR path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file) {
    if (opening != CpdFileOpening_Open && opening != CpdFileOpening_Create) {
        return CpdFileResult_InvalidCall;
    }

    CpdWindowsFile* win_file = (CpdWindowsFile*)malloc(sizeof(CpdWindowsFile));
    if (win_file == 0) {
        return CpdFileResult_OutOfMemory;
    }

    int path_size = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
    win_file->path = (char*)malloc(path_size);
    if (win_file->path == 0) {
        free(win_file);
        return CpdFileResult_OutOfMemory;
    }

    WideCharToMultiByte(CP_UTF8, 0, path, -1, win_file->path, path_size, NULL, NULL);

    DWORD access_flags;
    switch (mode) {
        case CpdFileMode_Read:
            access_flags = GENERIC_READ;
            break;

        case CpdFileMode_Write:
            access_flags = GENERIC_WRITE;
            break;

        case CpdFileMode_ReadWrite:
            access_flags = GENERIC_READ | GENERIC_WRITE;
            break;

        default:
            free(win_file->path);
            free(win_file);
            return CpdFileResult_InvalidCall;
    }

    DWORD sharing_flags;
    switch (sharing) {
        case CpdFileSharing_Exclusive:
            sharing_flags = 0;
            break;

        case CpdFileSharing_ReadOnly:
            sharing_flags = FILE_SHARE_READ;
            break;

        case CpdFileSharing_ReadWrite:
            sharing_flags = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        default:
            free(win_file->path);
            free(win_file);
            return CpdFileResult_InvalidCall;
    }

    DWORD disposition = OPEN_EXISTING;
    if (opening == CpdFileOpening_Create) {
        disposition = CREATE_ALWAYS;
    }

    win_file->handle = CreateFileW(path, access_flags, sharing_flags, 0, disposition, FILE_ATTRIBUTE_NORMAL, 0);
    if (win_file->handle == 0) {
        free(win_file->path);
        free(win_file);
        switch (GetLastError()) {
            case __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
                return CpdFileResult_InvalidPath;

            case __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
                return CpdFileResult_NoPermission;

            default:
                return CpdFileResult_InvalidCall;
        }
    }

    win_file->mode = mode;
    win_file->sharing = sharing;
    *file = (CpdFile)win_file;

    return CpdFileResult_Success;
}

CpdFileResult open_file(const char* path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file) {
    if (opening != CpdFileOpening_Open && opening != CpdFileOpening_Create) {
        return CpdFileResult_InvalidCall;
    }

    CpdWindowsFile* win_file = (CpdWindowsFile*)malloc(sizeof(CpdWindowsFile));
    if (win_file == 0) {
        return CpdFileResult_OutOfMemory;
    }

    size_t path_size = strlen(path);
    win_file->path = (char*)malloc((path_size + 1) * sizeof(char));
    if (win_file->path == 0) {
        free(win_file);
        return CpdFileResult_OutOfMemory;
    }

    strcpy(win_file->path, path);

    DWORD access_flags;
    switch (mode) {
        case CpdFileMode_Read:
            access_flags = GENERIC_READ;
            break;

        case CpdFileMode_Write:
            access_flags = GENERIC_WRITE;
            break;

        case CpdFileMode_ReadWrite:
            access_flags = GENERIC_READ | GENERIC_WRITE;
            break;

        default:
            free(win_file->path);
            free(win_file);
            return CpdFileResult_InvalidCall;
    }

    DWORD sharing_flags;
    switch (sharing) {
        case CpdFileSharing_Exclusive:
            sharing_flags = 0;
            break;

        case CpdFileSharing_ReadOnly:
            sharing_flags = FILE_SHARE_READ;
            break;

        case CpdFileSharing_ReadWrite:
            sharing_flags = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        default:
            free(win_file->path);
            free(win_file);
            return CpdFileResult_InvalidCall;
    }

    DWORD disposition = OPEN_EXISTING;
    if (opening == CpdFileOpening_Create) {
        disposition = CREATE_ALWAYS;
    }

    win_file->handle = CreateFileA(path, access_flags, sharing_flags, 0, disposition, FILE_ATTRIBUTE_NORMAL, 0);
    if (win_file->handle == 0) {
        free(win_file->path);
        free(win_file);
        switch (GetLastError()) {
            case __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
                return CpdFileResult_InvalidPath;

            case __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
                return CpdFileResult_NoPermission;

            default:
                return CpdFileResult_InvalidCall;
        }
    }

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

    LONG high = 0;
    DWORD result = SetFilePointer(win_file->handle, 0, &high, FILE_CURRENT);
    if (result == INVALID_SET_FILE_POINTER && GetLastError() == INVALID_SET_FILE_POINTER) {
        return -1;
    }

    return (int64_t)((uint64_t)high << 32) | result;
}

bool set_file_pos(CpdFile file, CpdFileOffsetType type, int64_t value) {
    if ((uint32_t)type > CpdFileOffsetType_End) {
        return false;
    }

    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    LONG high = (LONG)((uint64_t)value >> 32);
    return SetFilePointer(win_file->handle, (LONG)value, &high, (DWORD)type) != INVALID_SET_FILE_POINTER || GetLastError() != INVALID_SET_FILE_POINTER;
}

CpdFileResult read_file(CpdFile file, void* buffer, uint32_t* length) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    BOOL result = ReadFile(win_file->handle, buffer, *length, length, 0);

    return result ? CpdFileResult_Success : CpdFileResult_InvalidCall;
}

CpdFileResult write_file(CpdFile file, void* buffer, uint32_t* length) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    BOOL result = WriteFile(win_file->handle, buffer, *length, length, 0);
    
    return result ? CpdFileResult_Success : CpdFileResult_InvalidCall;
}

void close_file(CpdFile file) {
    CpdWindowsFile* win_file = (CpdWindowsFile*)file;

    CloseHandle(win_file->handle);
    free(win_file->path);
    free(win_file);
}
