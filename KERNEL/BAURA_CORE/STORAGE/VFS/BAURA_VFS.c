#include "BAURA_VFS.h"
#include "../VOLUME_MANAGER/volume_manager.h"

int baura_vfs_resolve_path(const char *baura_path, BauraPathInfo *info) {
    char volume_name[BAURA_VOLUME_NAME_MAX];
    uint32_t volume_id;
    uint32_t path_length = 0;
    uint32_t volume_length = 0;
    uint32_t input_offset;
    uint32_t output_length = 0;
    char normalized[BAURA_VFS_MAX_PATH];
    int status;

    if (baura_path == 0 || info == 0) return BAURA_VFS_ERR_INVALID_ARGUMENT;
    while (path_length < BAURA_VFS_MAX_PATH && baura_path[path_length] != '\0') {
        ++path_length;
    }
    if (path_length == 0) return BAURA_VFS_ERR_INVALID_PATH;
    if (path_length >= BAURA_VFS_MAX_PATH) return BAURA_VFS_ERR_PATH_TOO_LONG;
    if (path_length < 6u || baura_path[0] != 'P' || baura_path[1] != 'C' ||
        baura_path[2] != '/') {
        return BAURA_VFS_ERR_INVALID_PATH;
    }

    input_offset = 3u;
    while (input_offset < path_length && baura_path[input_offset] != ':') {
        char value = baura_path[input_offset];
        if (volume_length + 1u >= BAURA_VOLUME_NAME_MAX ||
            !((value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '_')) {
            return BAURA_VFS_ERR_INVALID_PATH;
        }
        volume_name[volume_length++] = value;
        ++input_offset;
    }
    if (volume_length == 0 || input_offset + 1u >= path_length ||
        baura_path[input_offset] != ':' || baura_path[input_offset + 1u] != '/') {
        return BAURA_VFS_ERR_INVALID_PATH;
    }
    volume_name[volume_length] = '\0';
    status = baura_volume_get_id(volume_name, &volume_id);
    if (status == BAURA_ERR_NOT_FOUND) return BAURA_VFS_ERR_VOLUME_NOT_FOUND;
    if (status != BAURA_OK) return BAURA_VFS_ERR_INVALID_PATH;

    input_offset += 2u;
    while (input_offset < path_length) {
        uint32_t component_start;
        uint32_t component_length;
        uint32_t component_index;
        while (input_offset < path_length && baura_path[input_offset] == '/') {
            ++input_offset;
        }
        if (input_offset == path_length) break;

        component_start = input_offset;
        while (input_offset < path_length && baura_path[input_offset] != '/') {
            unsigned char value = (unsigned char)baura_path[input_offset];
            if (value < 0x20u || baura_path[input_offset] == '\\' ||
                baura_path[input_offset] == ':') {
                return BAURA_VFS_ERR_INVALID_PATH;
            }
            ++input_offset;
        }
        component_length = input_offset - component_start;
        if (component_length == 1u && baura_path[component_start] == '.') continue;
        if (component_length == 2u && baura_path[component_start] == '.' &&
            baura_path[component_start + 1u] == '.') {
            return BAURA_VFS_ERR_INVALID_PATH;
        }

        if (output_length != 0) {
            if (output_length >= BAURA_VFS_MAX_PATH - 1u) {
                return BAURA_VFS_ERR_PATH_TOO_LONG;
            }
            normalized[output_length++] = '/';
        }
        if (component_length > BAURA_VFS_MAX_PATH - 1u - output_length) {
            return BAURA_VFS_ERR_PATH_TOO_LONG;
        }
        for (component_index = 0; component_index < component_length;
             ++component_index) {
            normalized[output_length++] = baura_path[component_start + component_index];
        }
    }

    for (input_offset = 0; input_offset < output_length; ++input_offset) {
        info->path[input_offset] = normalized[input_offset];
    }
    info->path[output_length] = '\0';
    info->volume_id = volume_id;
    info->path_len = output_length;
    return BAURA_VFS_OK;
}
