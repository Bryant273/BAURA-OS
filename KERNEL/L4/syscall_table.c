
#include <stdint.h>
#include "BAURA_VFS_RUST.h"
#include "BAURA_SECURITY_RUST.h"
typedef void* (*syscall_t)(void*);
typedef struct {
    syscall_t handler;
    uint32_t privilege_level;
} SyscallEntry;
SyscallEntry syscall_table[] = {
    [0] = { (syscall_t)vfs_resolve_path, 1 },
    [1] = { (syscall_t)sec_validate_capability, 0 },
};
void handle_syscall(uint32_t id, void* args) {
    if (id < (sizeof(syscall_table)/sizeof(SyscallEntry)) && syscall_table[id].handler) {
        syscall_table[id].handler(args);
    }
}
