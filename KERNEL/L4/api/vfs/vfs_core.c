
#include <stdint.h>
#include <string.h>
#include "BAURA_API.h"
#include "L4_TCB.h"

// Internal state for open files
typedef struct {
    uint32_t inode;
    uint64_t offset;
    uint32_t flags;
    uint32_t uid;
    uint32_t gid;
} FileHandle;

// API 101: BAURA_FS_OPEN
int api_fs_open(const char* path, uint32_t flags) {
    if (!path) return ERR_INVALID_PATH;

    // 1. Path Resolution (VFS Layer)
    uint32_t inode = vfs_resolve_path(path);
    if (inode == ERR_NOT_FOUND) {
        kernel_log("VFS: File not found at path %s", path);
        return ERR_FILE_NOT_FOUND;
    }

    // 2. Permission Check (Security Layer)
    if (!sec_validate_access(current_task->id, inode, flags)) {
        kernel_log("SECURITY: Access denied to inode %d for task %d", inode, current_task->pid);
        return ERR_PERMISSION_DENIED;
    }

    // 3. Handle Allocation via Slab Cache
    FileHandle* fh = slab_alloc(sizeof(FileHandle));
    if (!fh) return ERR_OUT_OF_MEMORY;

    fh->inode = inode;
    fh->offset = 0;
    fh->flags = flags;
    fh->uid = current_task->uid;
    fh->gid = current_task->gid;

    return assign_fd(fh);
}

// API 112: BAURA_FS_READ_ATOMIC
size_t api_fs_read_atomic(int fd, void* buf, size_t size, uint64_t offset) {
    FileHandle* fh = get_fd(fd);
    if (!fh) return ERR_INVALID_FD;

    // Validate boundaries to prevent Buffer Overflows
    uint64_t file_size = baurafs_get_size(fh->inode);
    if (offset >= file_size) return 0;

    size_t actual_read = (offset + size > file_size) ? (file_size - offset) : size;

    // Direct call to BAURAFS Atomic Read
    return baurafs_read_block(fh->inode, offset, buf, actual_read);
}
