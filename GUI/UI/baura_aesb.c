/**
 * BAURA OS — BUI SDK
 * baura_aesb.c — Implémentation du chargeur et exportateur de paquets .AESB
 */

#include "baura_aesb.h"
#include <string.h>

bool baura_aesb_validate_header(const baura_aesb_header_t* header) {
    if (!header) return false;
    if (header->magic != BAURA_AESB_MAGIC) return false;
    if (header->version != BAURA_AESB_VERSION) return false;
    if (header->app_id[0] == '\0') return false;
    return true;
}

bool baura_aesb_init_header_from_desc(baura_aesb_header_t* header, const bui_app_desc_t* desc, uint32_t perms) {
    if (!header || !desc) return false;

    memset(header, 0, sizeof(baura_aesb_header_t));
    header->magic = BAURA_AESB_MAGIC;
    header->version = BAURA_AESB_VERSION;
    header->arch = 1; /* x86_64 */

    strncpy(header->app_id, desc->app_id ? desc->app_id : "", sizeof(header->app_id) - 1);
    strncpy(header->name, desc->name ? desc->name : "", sizeof(header->name) - 1);
    strncpy(header->version_str, desc->version ? desc->version : "1.0.0", sizeof(header->version_str) - 1);
    strncpy(header->author, "Ahmed Silué", sizeof(header->author) - 1);
    strncpy(header->icon_name, desc->icon ? desc->icon : "app", sizeof(header->icon_name) - 1);

    header->role = (uint32_t)desc->role;
    header->permissions = perms;

    header->default_width_dp = desc->default_width_dp;
    header->default_height_dp = desc->default_height_dp;
    header->min_width_dp = desc->min_width_dp;
    header->min_height_dp = desc->min_height_dp;

    return true;
}

bool baura_aesb_register_in_runtime(const baura_aesb_header_t* header) {
    if (!baura_aesb_validate_header(header)) return false;

    /* Vérifier si l'application est déjà inscrite */
    if (bui_app_find_desc(header->app_id) != NULL) {
        return true;
    }

    return true;
}
