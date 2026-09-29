#ifndef BAURA_VFS_RUST_H
#define BAURA_VFS_RUST_H

#include "BAURA_VFS.h"

#ifdef __cplusplus
extern "C" {
#endif

int baura_vfs_resolve_path_rust(const char *baura_path, BauraPathInfo *info);

#ifdef __cplusplus
}
#endif

#endif
