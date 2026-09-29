
#include <stdint.h>
#include "BAURA_VMM.h"

// API 130: BAURA_FS_WRITE_ATOMIC
int api_fs_write_atomic(int fd, void* buf, size_t size, uint64_t offset) {
    FileHandle* fh = get_fd(fd);
    if (!fh) return ERR_INVALID_FD;

    // SHADOW PAGING LOGIC:
    // Instead of overwriting data, we create a new block to ensure atomicity.

    // 1. Allocate a new physical block via PMM
    uintptr_t new_block_phys = pmm_alloc_page();
    if (!new_block_phys) return ERR_OUT_OF_MEMORY;

    // 2. Copy current data + new data into the shadow block
    void* shadow_buf = vmm_map_physical(new_block_phys);
    baurafs_prepare_shadow_block(fh->inode, offset, shadow_buf, buf, size);

    // 3. Commit the change: Update Inode to point to the new block
    // This is the ONLY point of failure; if it fails, the original data is untouched.
    if (baurafs_commit_inode_change(fh->inode, offset, new_block_phys) != SUCCESS) {
        pmm_free_page(new_block_phys);
        return ERR_DISK_FAILURE;
    }

    return SUCCESS;
}
