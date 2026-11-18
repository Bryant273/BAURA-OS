/**
 * BAURA OS — BUI (Baura User Interface) SDK
 * bui_app.h — Structure et Modèle d'Application Standardisée pour BAURA OS
 * 
 * Selon la vision d'Ahmed Silué :
 * "Aussi évite de coder des applis en dur dès le départ... Lorsqu'on aura un truc solide
 *  on va coder les applis... Aussi on peut considérer les éléments de l'interface comme
 *  des applis pour une meilleure prise en main et maintenance, on doit donc définir un SDK
 *  et si ça passe bien c'est ça qui va devenir notre structure d'appli pour notre os"
 */

#ifndef BAURA_BUI_APP_H
#define BAURA_BUI_APP_H

#include "bui_types.h"
#include "bui_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Rôles d'application dans le système BAURA OS */
typedef enum {
    BUI_APP_ROLE_WINDOW = 0,    /* Application standard fenêtrée (Fichiers, Terminal, Moniteur) */
    BUI_APP_ROLE_PANEL,         /* Élément de panneau système persistant (Split-Dock, Top Tray) */
    BUI_APP_ROLE_POPUP,         /* Surface transitoire volante (Menu BStart, Menu Contextuel) */
    BUI_APP_ROLE_OVERLAY        /* Couche supérieure plein écran (Verrouillage, Alerte modale) */
} bui_app_role_t;

/* Déclaration anticipée de l'instance */
struct bui_app_instance;

/* Descripteur d'application (Manifeste d'enregistrement) */
typedef struct bui_app_desc {
    const char*     app_id;             /* Identifiant unique (ex: "org.baura.files", "org.baura.terminal") */
    const char*     name;               /* Nom affiché (ex: "Fichiers", "Terminal") */
    const char*     version;            /* Version semver (ex: "1.0.0") */
    const char*     icon;               /* Nom de l'icône vectorielle (ex: "folder", "terminal") */
    bui_app_role_t  role;               /* Fenêtre, Panneau Dock, Menu volant */
    
    /* Géométrie par défaut (en dp) */
    bui_dp_t        default_width_dp;
    bui_dp_t        default_height_dp;
    bui_dp_t        min_width_dp;
    bui_dp_t        min_height_dp;
    
    /* Points d'entrée du cycle de vie de l'application (SDK Hooks) */
    bool (*init)(struct bui_app_instance* app);
    void (*build_ui)(struct bui_app_instance* app, bui_widget_t* root_container);
    void (*on_event)(struct bui_app_instance* app, const bui_event_t* event);
    void (*on_command)(struct bui_app_instance* app, const char* command, const void* payload);
    void (*on_tick)(struct bui_app_instance* app, uint32_t delta_ms);
    void (*destroy)(struct bui_app_instance* app);
} bui_app_desc_t;

/* Instance en cours d'exécution d'une application BUI */
typedef struct bui_app_instance {
    const bui_app_desc_t* desc;
    uint32_t              instance_id;
    bool                  is_running;
    bool                  is_visible;
    bool                  is_focused;

    /* Arbre de widgets de l'application */
    bui_widget_t*         root_widget;        /* Conteneur racine */
    bui_widget_t*         window_frame;       /* Si fenêtré, pointe vers le cadre */
    
    /* Contexte privé de l'application */
    void*                 user_state;
    
    /* Rectangle actuel alloué par le compositeur (en dp) */
    bui_rect_t            current_rect;
    
    /* Gestion des listes chaînées d'instances actives */
    struct bui_app_instance* next;
} bui_app_instance_t;

/* Gestionnaire du Registre d'Applications BAURA OS */
bool                bui_app_register(const bui_app_desc_t* desc);
const bui_app_desc_t* bui_app_find_desc(const char* app_id);
uint32_t            bui_app_get_registered_count(void);
const bui_app_desc_t* bui_app_get_desc_by_index(uint32_t index);

/* Gestion du Cycle de Vie des Instances */
bui_app_instance_t* bui_app_launch(const char* app_id);
void                bui_app_terminate(bui_app_instance_t* instance);
void                bui_app_broadcast_tick(uint32_t delta_ms);
void                bui_app_dispatch_event_to_focused(const bui_event_t* event);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BUI_APP_H */
