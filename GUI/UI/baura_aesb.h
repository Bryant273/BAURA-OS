/**
 * BAURA OS — BUI SDK & Executable Application Model
 * baura_aesb.h — Définition et manipulation du format de paquet .AESB (Appli Exécutable Sur BAURA)
 */

#ifndef BAURA_AESB_H
#define BAURA_AESB_H

#include "bui_types.h"
#include "bui_app.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAURA_AESB_MAGIC 0x42534541 /* "AESB" */
#define BAURA_AESB_VERSION 1

/* Permissions système déclarées dans le manifeste .aesb */
typedef enum {
    AESB_PERM_NONE        = 0,
    AESB_PERM_VFS_READ    = (1 << 0), /* Lecture fichiers PC/<LECTEUR>:/... */
    AESB_PERM_VFS_WRITE   = (1 << 1), /* Écriture fichiers */
    AESB_PERM_GUI_SURFACE = (1 << 2), /* Création de surfaces d'affichage BDS */
    AESB_PERM_INPUT_EVT   = (1 << 3), /* Réception clavier et souris */
    AESB_PERM_IPC_BUS     = (1 << 4), /* Communication sur le Message Bus */
    AESB_PERM_NETWORK     = (1 << 5), /* Accès pile réseau TCP/IP */
    AESB_PERM_DEVICE_RAW  = (1 << 6), /* Accès direct périphériques */
    AESB_PERM_SYS_ADMIN   = (1 << 7)  /* Élévation administrateur */
} baura_aesb_permission_flags_t;

/* Rôle d'interface de l'exécutable .aesb */
typedef enum {
    AESB_ROLE_DESKTOP_APP = 0, /* Fenêtre standard (Fichiers, Terminal) */
    AESB_ROLE_SHELL_PANEL = 1, /* Composant Shell (Split-Dock, TopBar) */
    AESB_ROLE_POPUP_MENU  = 2, /* Menu volant transitoire (BStart) */
    AESB_ROLE_BACKGROUND  = 3  /* Service de fond / Démon */
} baura_aesb_role_t;

/* En-tête binaire officiel du conteneur .aesb */
typedef struct {
    uint32_t magic;             /* 'AESB' */
    uint16_t version;           /* 1 */
    uint16_t arch;              /* 1 = x86_64 */
    
    char     app_id[64];        /* Identifiant unique (ex: "org.baura.apps.files") */
    char     name[48];          /* Nom affiché (ex: "Fichiers") */
    char     version_str[16];   /* Version semver (ex: "1.0.0") */
    char     author[48];        /* Auteur (ex: "Ahmed Silué") */
    char     icon_name[32];     /* Icône (ex: "folder", "terminal") */

    uint32_t role;              /* baura_aesb_role_t */
    uint32_t permissions;       /* Masque baura_aesb_permission_flags_t */

    /* Géométrie par défaut de l'interface en dp */
    int32_t  default_width_dp;
    int32_t  default_height_dp;
    int32_t  min_width_dp;
    int32_t  min_height_dp;

    /* Offsets et tailles des sections */
    uint32_t manifest_offset;
    uint32_t manifest_size;
    uint32_t code_offset;
    uint32_t code_size;
    uint32_t assets_offset;
    uint32_t assets_size;
    uint32_t signature_offset;
    uint32_t signature_size;

    uint32_t entry_point_offset;
    uint32_t checksum_crc32;
    uint8_t  reserved[32];
} __attribute__((packed)) baura_aesb_header_t;

/* Validation et parsing */
bool baura_aesb_validate_header(const baura_aesb_header_t* header);
bool baura_aesb_init_header_from_desc(baura_aesb_header_t* header, const bui_app_desc_t* desc, uint32_t perms);

/* Enregistrement automatique d'un binaire .aesb dans le SDK BUI */
bool baura_aesb_register_in_runtime(const baura_aesb_header_t* header);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_AESB_H */
