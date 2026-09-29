#include "BAURA_VFS.h"
#include "BAURA_VFS_RUST.h"

int baura_vfs_resolve_path(const char *baura_path, BauraPathInfo *info) {
    return baura_vfs_resolve_path_rust(baura_path, info);
}
