/**
 * BAURA OS — BUI (Baura User Interface) SDK
 * bui_widget.h — Définition de l'arbre de widgets et des composants standards
 */

#ifndef BAURA_BUI_WIDGET_H
#define BAURA_BUI_WIDGET_H

#include "bui_types.h"
#include "bui_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Types de widgets de base du SDK BUI */
typedef enum {
    BUI_WIDGET_CONTAINER = 0,
    BUI_WIDGET_LABEL,
    BUI_WIDGET_BUTTON,
    BUI_WIDGET_ICON_BUTTON,
    BUI_WIDGET_ICON,
    BUI_WIDGET_PANEL,        /* Panneau de verre (Split-dock, Tray, Carte) */
    BUI_WIDGET_WINDOW_FRAME, /* Cadre de fenêtre avec barre de titre */
    BUI_WIDGET_TEXT_INPUT,
    BUI_WIDGET_DIVIDER,
    BUI_WIDGET_CUSTOM
} bui_widget_type_t;

/* États d'interaction d'un widget */
typedef enum {
    BUI_STATE_NORMAL   = 0,
    BUI_STATE_HOVER    = (1 << 0),
    BUI_STATE_PRESSED  = (1 << 1),
    BUI_STATE_FOCUSED  = (1 << 2),
    BUI_STATE_DISABLED = (1 << 3)
} bui_widget_state_flags_t;

/* Types d'événements utilisateur acheminés par BUI */
typedef enum {
    BUI_EVENT_NONE = 0,
    BUI_EVENT_MOUSE_MOVE,
    BUI_EVENT_MOUSE_DOWN,
    BUI_EVENT_MOUSE_UP,
    BUI_EVENT_MOUSE_CLICK,
    BUI_EVENT_MOUSE_DRAG,
    BUI_EVENT_KEY_DOWN,
    BUI_EVENT_KEY_UP,
    BUI_EVENT_FOCUS_GAINED,
    BUI_EVENT_FOCUS_LOST
} bui_event_type_t;

/* Structure d'événement */
typedef struct {
    bui_event_type_t type;
    bui_point_t      pos;         /* Coordonnées du curseur en dp */
    bui_dp_t         delta_x;     /* Déplacement souris dx */
    bui_dp_t         delta_y;     /* Déplacement souris dy */
    uint32_t         button_mask; /* Boutons souris (1=gauche, 2=droit, 4=milieu) */
    uint32_t         key_code;    /* Code touche clavier */
    uint32_t         modifiers;   /* Shift, Ctrl, Alt */
} bui_event_t;

/* Déclaration du widget */
typedef struct bui_widget {
    uint32_t            id;
    bui_widget_type_t   type;
    uint32_t            state;           /* Masque bui_widget_state_flags_t */
    bool                visible;
    bool                interactive;

    /* Géométrie calculée par le layout engine (unités dp) */
    bui_size_t          measured_size;
    bui_rect_t          bounds;
    bui_dp_t            flex_grow;       /* 0 = taille propre, 1+ = s'étire pour occuper l'espace */

    /* Paramètres de mise en page si ce widget est un conteneur */
    bui_layout_params_t layout_params;

    /* Données de style et contenu */
    char                text[128];
    char                icon[32];
    bui_color_t         bg_color;
    bui_color_t         fg_color;
    bui_dp_t            corner_radius;
    bool                is_glass;

    /* Arbre hiérarchique */
    struct bui_widget*  parent;
    struct bui_widget*  first_child;
    struct bui_widget*  last_child;
    struct bui_widget*  next_sibling;
    struct bui_widget*  prev_sibling;
    uint32_t            child_count;

    /* Callbacks d'événements */
    void (*on_click)(struct bui_widget* w, const bui_event_t* evt, void* user_data);
    void (*on_hover)(struct bui_widget* w, bool entered, void* user_data);
    void* user_data;
} bui_widget_t;

/* Gestion du cycle de vie des widgets */
bui_widget_t* bui_widget_create(bui_widget_type_t type, uint32_t id);
void          bui_widget_destroy(bui_widget_t* widget);
void          bui_widget_add_child(bui_widget_t* parent, bui_widget_t* child);
void          bui_widget_remove_child(bui_widget_t* parent, bui_widget_t* child);

/* Constructeurs de haut niveau (Widgets déclaratifs) */
bui_widget_t* bui_create_row(bui_dp_t gap, bui_insets_t padding);
bui_widget_t* bui_create_column(bui_dp_t gap, bui_insets_t padding);
bui_widget_t* bui_create_panel(bui_dp_t corner_radius, bool is_glass);
bui_widget_t* bui_create_label(const char* text, bui_color_t color);
bui_widget_t* bui_create_button(const char* label, void (*on_click)(bui_widget_t*, const bui_event_t*, void*));
bui_widget_t* bui_create_icon_button(const char* icon_name, const char* tooltip);
bui_widget_t* bui_create_divider(bui_dp_t length, bool vertical);
bui_widget_t* bui_create_window_frame(const char* title, bui_dp_t width_dp, bui_dp_t height_dp);

/* Routage des événements dans l'arbre */
bui_widget_t* bui_widget_hit_test(bui_widget_t* root, bui_point_t pt);
bool          bui_widget_dispatch_event(bui_widget_t* root, const bui_event_t* evt);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BUI_WIDGET_H */
