/**
 * BAURA OS — BUI (Baura User Interface) SDK
 * bui_types.h — Types de base, géométrie en unités logiques (dp), et jetons de thème
 * 
 * Conforme aux spécifications :
 * - "Aucune coordonnée écrite en dur" (Arbre de widgets et layout 2 passes).
 * - "Unités logiques (dp), pas des pixels" (Conversion dpi / scale en bout de chaîne).
 * - Charte graphique BAURA OS (Styles_composants_ui, verre dépoli, accent 1/2).
 */

#ifndef BAURA_BUI_TYPES_H
#define BAURA_BUI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Unité logique de mise en page (Device Independent Pixels) */
typedef int32_t bui_dp_t;

/* Point logique */
typedef struct {
    bui_dp_t x;
    bui_dp_t y;
} bui_point_t;

/* Taille logique */
typedef struct {
    bui_dp_t width;
    bui_dp_t height;
} bui_size_t;

/* Rectangle logique */
typedef struct {
    bui_dp_t x;
    bui_dp_t y;
    bui_dp_t width;
    bui_dp_t height;
} bui_rect_t;

/* Marges et espacements intérieurs (Insets/Padding) */
typedef struct {
    bui_dp_t top;
    bui_dp_t right;
    bui_dp_t bottom;
    bui_dp_t left;
} bui_insets_t;

/* Contraintes de dimensionnement pour la passe 'measure' */
typedef struct {
    bui_dp_t min_width;
    bui_dp_t max_width;
    bui_dp_t min_height;
    bui_dp_t max_height;
} bui_constraints_t;

/* Couleur ARGB 32 bits */
typedef uint32_t bui_color_t;

#define BUI_COLOR_ARGB(a, r, g, b) \
    (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define BUI_COLOR_RGB(r, g, b) \
    BUI_COLOR_ARGB(0xFF, (r), (g), (b))

/* Jetons de couleur conformes au design system BAURA OS */
typedef struct {
    bui_color_t accent_1;       /* --a1 : Bleu royal BAURA #1D4ED8 */
    bui_color_t accent_2;       /* --a2 : Cyan lumineux #38BDF8 */
    bui_color_t bg_night;       /* --bg : Nuit Indigo #0B0F19 */
    bui_color_t glass_bg;       /* --glass-strong : Verre 65% noir/bleu #101827A6 */
    bui_color_t glass_border;   /* --glass-line : Liseré blanc translucide #FFFFFF26 */
    bui_color_t hairline;       /* --hair : Filet de séparation #FFFFFF1A */
    bui_color_t fg_primary;     /* --fg : Blanc texte #F9FAFB */
    bui_color_t fg_secondary;   /* --fg-sec : Gris neutre #9CA3AF */
    bui_color_t fg_muted;       /* --mut : Gris sombre #4B5563 */
    bui_color_t surface_field;  /* --field : Fond champ texte/item #1F293780 */
    bui_color_t hover_overlay;  /* --hover : Blanc 8% survol #FFFFFF14 */
    bui_color_t press_overlay;  /* --press : Blanc 14% pressé #FFFFFF24 */
    bui_color_t error_color;    /* --err : Rouge alerte #EF4444 */
    bui_color_t success_color;  /* --ok : Vert émeraude #10B981 */
} bui_theme_colors_t;

/* Jetons de rayons d'arrondis */
typedef struct {
    bui_dp_t radius_xs;  /* 4 dp  - tags, petites pilules */
    bui_dp_t radius_sm;  /* 6 dp  - boutons compacts, champs */
    bui_dp_t radius_md;  /* 10 dp - boutons standards, cartes */
    bui_dp_t radius_lg;  /* 14 dp - fenêtres, panneaux */
    bui_dp_t radius_xl;  /* 20 dp - capsules dock, dialogues */
    bui_dp_t radius_full;/* 999 dp - cercles complets */
} bui_theme_radii_t;

/* Jetons d'espacement (Spacing grid 4 dp) */
typedef struct {
    bui_dp_t space_1;   /* 4 dp  */
    bui_dp_t space_2;   /* 8 dp  */
    bui_dp_t space_3;   /* 12 dp */
    bui_dp_t space_4;   /* 16 dp */
    bui_dp_t space_5;   /* 24 dp */
    bui_dp_t space_6;   /* 32 dp */
} bui_theme_spacing_t;

/* Thème global BUI actif */
typedef struct {
    bui_theme_colors_t  colors;
    bui_theme_radii_t   radii;
    bui_theme_spacing_t spacing;
    uint32_t            scale_factor_pct; /* 100% = 1.0x, 125% = 1.25x, 200% = 2.0x */
} bui_theme_t;

/* Fonctions d'accès au thème */
const bui_theme_t* bui_theme_default(void);
bui_rect_t bui_rect_make(bui_dp_t x, bui_dp_t y, bui_dp_t w, bui_dp_t h);
bool bui_rect_contains(const bui_rect_t* rect, bui_point_t pt);
bui_rect_t bui_rect_inset(const bui_rect_t* rect, bui_insets_t insets);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BUI_TYPES_H */
