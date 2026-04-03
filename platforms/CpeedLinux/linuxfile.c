#include <Cpeed/platform/file.h>

#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

typedef struct CpdLinuxFile {
    struct flock flock;
    char* path;
    int fd;
    CpdFileMode mode;
    CpdFileSharing sharing;
} CpdLinuxFile;

CpdFileResult file_exists(const char* path) {
    return access(path, F_OK) == 0 ? CpdFileResult_Success : CpdFileResult_InvalidPath;
}

CpdFileResult open_file(const char* path, CpdFileOpening opening, CpdFileMode mode, CpdFileSharing sharing, CpdFile* file) {
    if (opening != CpdFileOpening_Open && opening != CpdFileOpening_Create) {
        return CpdFileResult_InvalidCall;
    }

    CpdLinuxFile* linux_file = (CpdLinuxFile*)malloc(sizeof(CpdLinuxFile));
    if (linux_file == 0) {
        return CpdFileResult_OutOfMemory;
    }

    size_t path_size = strlen(path);
    linux_file->path = (char*)malloc((path_size + 1) * sizeof(char));
    if (linux_file->path == 0) {
        free(linux_file);
        return CpdFileResult_OutOfMemory;
    }

    strcpy(linux_file->path, path);

    int flags = 0;
    switch (mode) {
        case CpdFileMode_Read:
            flags |= O_RDONLY;
            break;

        case CpdFileMode_Write:
            flags |= O_WRONLY;
            break;

        case CpdFileMode_ReadWrite:
            flags |= O_RDWR;
            break;

        default:
            free(linux_file->path);
            free(linux_file);
            return CpdFileResult_InvalidCall;
    }

    linux_file->flock.l_whence = SEEK_SET;
    linux_file->flock.l_start = 0;
    linux_file->flock.l_len = (off_t)UINT64_MAX;

    switch (sharing) {
        case CpdFileSharing_Exclusive:
            linux_file->flock.l_type = F_RDLCK;
            break;

        case CpdFileSharing_ReadOnly:
            linux_file->flock.l_type = F_WRLCK;
            break;

        case CpdFileSharing_ReadWrite:
            break;

        default:
            free(linux_file->path);
            free(linux_file);
            return CpdFileResult_InvalidCall;
    }

    if (opening == CpdFileOpening_Create) {
        flags |= O_CREAT;
    }
    
    linux_file->fd = openat(AT_FDCWD, path, flags);
    if (linux_file->fd == -1) {
        free(linux_file->path);
        free(linux_file);
        switch (errno) {
            case ENOENT:
                return CpdFileResult_InvalidPath;

            case EACCES:
                return CpdFileResult_NoPermission;

            default:
                return CpdFileResult_InvalidCall;
        }
    }

    if (sharing != CpdFileSharing_ReadWrite) {
        fcntl(linux_file->fd, F_SETLK, &linux_file->flock);
    }

    linux_file->mode = mode;
    linux_file->sharing = sharing;
    *file = (CpdFile)linux_file;

    return CpdFileResult_Success;
}

const char* file_path(CpdFile file) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    return linux_file->path;
}

CpdFileMode file_mode(CpdFile file) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    return linux_file->mode;
}

CpdFileSharing file_sharing(CpdFile file) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    return linux_file->sharing;
}

int64_t file_pos(CpdFile file) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    off_t pos = lseek(linux_file->fd, 0, SEEK_CUR);

    return pos;
}

bool set_file_pos(CpdFile file, CpdFileOffsetType type, int64_t value) {
    if ((uint32_t)type > CpdFileOffsetType_End) {
        return false;
    }

    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    return lseek(linux_file->fd, value, (int)type) >= 0;
}

CpdFileResult read_file(CpdFile file, void* buffer, uint32_t* length) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    size_t length_size = *length;
    ssize_t signed_length = read(linux_file->fd, buffer, length_size);
    *length = signed_length;

    return signed_length >= 0 ? CpdFileResult_Success : CpdFileResult_InvalidCall;
}

CpdFileResult write_file(CpdFile file, void* buffer, uint32_t* length) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    size_t length_size = *length;
    ssize_t signed_length = write(linux_file->fd, buffer, length_size);
    *length = signed_length;

    return signed_length >= 0 ? CpdFileResult_Success : CpdFileResult_InvalidCall;
}

void close_file(CpdFile file) {
    CpdLinuxFile* linux_file = (CpdLinuxFile*)file;

    if (linux_file->sharing != CpdFileSharing_ReadWrite) {
        linux_file->flock.l_type = F_UNLCK;
        fcntl(linux_file->fd, F_SETLK, &linux_file->flock);
    }

    close(linux_file->fd);
    free(linux_file->path);
    free(linux_file);
}
