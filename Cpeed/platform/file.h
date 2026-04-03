#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum CpdFileResult {
    CpdFileResult_Success,
    CpdFileResult_InvalidPath,
    CpdFileResult_NoPermission,
    CpdFileResult_OutOfMemory,
    CpdFileResult_InvalidCall,
    CpdFileResult_InternalError
} CpdFileResult;

typedef enum CpdFileOpening {
    CpdFileOpening_Open,
    CpdFileOpening_Create
} CpdFileOpening;

typedef enum CpdFileMode {
    CpdFileMode_Invalid,
    CpdFileMode_Read,
    CpdFileMode_Write,
    CpdFileMode_ReadWrite
} CpdFileMode;

typedef enum CpdFileSharing {
    CpdFileSharing_Exclusive,
    CpdFileSharing_ReadOnly,
    CpdFileSharing_ReadWrite
} CpdFileSharing;

typedef enum CpdFileOffsetType {
    CpdFileOffsetType_Begin,
    CpdFileOffsetType_Current,
    CpdFileOffsetType_End
} CpdFileOffsetType;

typedef void* CpdFile;

extern CpdFileResult file_exists(const char* path);

extern CpdFileResult open_file(const char* path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file);

extern const char* file_path(CpdFile file);

extern CpdFileMode file_mode(CpdFile file);

extern CpdFileSharing file_sharing(CpdFile file);

extern int64_t file_pos(CpdFile file);

extern bool set_file_pos(CpdFile file, CpdFileOffsetType type, int64_t value);

extern CpdFileResult read_file(CpdFile file, void* buffer, uint32_t* length); 

extern CpdFileResult write_file(CpdFile file, void* buffer, uint32_t* length);

extern void close_file(CpdFile file);
