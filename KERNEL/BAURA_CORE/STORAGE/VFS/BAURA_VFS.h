#ifndef BAURA_VFS_H
#define BAURA_VFS_H

#include <stdint.h>

#define BAURA_VFS_MAX_PATH 256u
#define BAURA_VFS_OK 0
#define BAURA_VFS_ERR_INVALID_ARGUMENT (-1)
#define BAURA_VFS_ERR_INVALID_PATH (-2)
#define BAURA_VFS_ERR_VOLUME_NOT_FOUND (-3)
#define BAURA_VFS_ERR_PATH_TOO_LONG (-4)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t volume_id;
    uint32_t path_len;
    char path[BAURA_VFS_MAX_PATH];
} BauraPathInfo;

int baura_vfs_resolve_path(const char *baura_path, BauraPathInfo *info);

#ifdef __cplusplus
}
#endif

#endif
