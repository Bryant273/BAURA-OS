/**
 * BAURA OS — BSP (Baura Surface Protocol)
 * bsp_protocol.h — Protocole d'échange entre BDS (Display Server) et les Applications BUI
 * 
 * Principe 3 du Cahier des Charges :
 * "Le compositeur ne connaît aucun bouton. Il compose des surfaces rectangulaires fournies
 *  par des processus. Barre, Démarrer et fenêtres sont des applications comme les autres."
 * 
 * Principe 4 du Cahier des Charges :
 * "Ne redessiner que ce qui a changé. Chaque couche transmet des rectangles de dégâts (damage) ;
 *  aucune ne repeint l'écran entier sans raison."
 */

#ifndef BAURA_BSP_PROTOCOL_H
#define BAURA_BSP_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_PROTOCOL_VERSION 1
#define BSP_MAGIC 0x42535031 /* 'BSP1' */

/* Rôles de surface assignés par le protocole BSP */
typedef enum {
    BSP_SURFACE_ROLE_NONE = 0,
    BSP_SURFACE_ROLE_TOPLEVEL,  /* Fenêtre d'application normale (Fichiers, Terminal) */
    BSP_SURFACE_ROLE_PANEL,     /* Barre des tâches / Split-Dock ancré */
    BSP_SURFACE_ROLE_POPUP,     /* Menu contextuel, menu Démarrer BStart */
    BSP_SURFACE_ROLE_OVERLAY,   /* Écran de veille / alerte de sécurité */
    BSP_SURFACE_ROLE_BACKGROUND /* Fond d'écran animé ou dynamique */
} bsp_surface_role_t;

/* Types de messages échangés sur le canal BSP */
typedef enum {
    BSP_MSG_INVALID = 0,
    
    /* Client -> BDS */
    BSP_MSG_CLIENT_HELLO,
    BSP_MSG_CREATE_SURFACE,
    BSP_MSG_DESTROY_SURFACE,
    BSP_MSG_SET_ROLE,
    BSP_MSG_SET_TITLE,
    BSP_MSG_ATTACH_BUFFER,
    BSP_MSG_DAMAGE_RECT,        /* Déclare une zone modifiée (x, y, w, h) */
    BSP_MSG_COMMIT,             /* Valide et présente la surface */
    
    /* BDS -> Client */
    BSP_MSG_SERVER_WELCOME,
    BSP_MSG_SURFACE_CONFIGURED, /* Indique la taille allouée et le scale factor */
    BSP_MSG_SURFACE_CLOSE_REQ,  /* Demande de fermeture (clic bouton rouge) */
    BSP_MSG_INPUT_EVENT,        /* Événement souris ou clavier routé */
    BSP_MSG_PING,
    BSP_MSG_PONG
} bsp_msg_type_t;

/* Rectangle de dégâts (Damage Rect) */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} bsp_rect_t;

/* En-tête universel de message BSP */
typedef struct {
    uint32_t       magic;          /* BSP_MAGIC */
    uint16_t       version;        /* BSP_PROTOCOL_VERSION */
    uint16_t       type;           /* bsp_msg_type_t */
    uint32_t       surface_id;     /* Identifiant de surface concernée */
    uint32_t       payload_size;   /* Taille des données suivantes en octets */
} bsp_header_t;

/* Payload : Création de surface */
typedef struct {
    uint32_t       role;           /* bsp_surface_role_t */
    int32_t        initial_w_dp;
    int32_t        initial_h_dp;
    char           title[64];
    char           app_id[64];
} bsp_msg_create_surface_t;

/* Payload : Déclaration de dommage et commit */
typedef struct {
    bsp_rect_t     damage;         /* Zone modifiée à redessiner */
    uint64_t       buffer_handle;  /* Référence au tampon partagé / SHM / seL4 Frame Cap */
    uint32_t       stride_bytes;
    uint32_t       pixel_format;   /* 0 = ARGB8888, 1 = RGBX8888 */
} bsp_msg_commit_t;

/* Payload : Configuration par le serveur */
typedef struct {
    int32_t        allocated_w_dp;
    int32_t        allocated_h_dp;
    uint32_t       scale_factor_pct; /* 100, 125, 150, 200 */
    bool           is_focused;
    bool           is_maximized;
} bsp_msg_configure_t;

/* Payload : Événement d'entrée routé */
typedef struct {
    uint32_t       event_type;
    int32_t        x_dp;
    int32_t        y_dp;
    int32_t        dx;
    int32_t        dy;
    uint32_t       buttons;
    uint32_t       keycode;
    uint32_t       modifiers;
} bsp_msg_input_t;

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BSP_PROTOCOL_H */
