/**
 * BAURA OS — BUI (Baura User Interface) SDK
 * bui_layout.h — Moteur de mise en page automatique en deux passes (measure & layout)
 * 
 * Principe 1 du Cahier des Charges :
 * "Aucune coordonnée écrite en dur. Toute interface est un arbre de widgets dont la mise en page
 *  est calculée (taille souhaitée, puis placement). C'est ce qui supprime l'effet 'dessiné à la main'."
 */

#ifndef BAURA_BUI_LAYOUT_H
#define BAURA_BUI_LAYOUT_H

#include "bui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Orientation du conteneur de flux */
typedef enum {
    BUI_LAYOUT_ROW = 0,    /* Disposition horizontale (enfants côte à côte) */
    BUI_LAYOUT_COLUMN,     /* Disposition verticale (enfants empilés) */
    BUI_LAYOUT_STACK       /* Superposition (couches superposées Z-order) */
} bui_layout_direction_t;

/* Alignement sur l'axe principal */
typedef enum {
    BUI_ALIGN_START = 0,
    BUI_ALIGN_CENTER,
    BUI_ALIGN_END,
    BUI_ALIGN_SPACE_BETWEEN
} bui_main_align_t;

/* Alignement sur l'axe transversal */
typedef enum {
    BUI_CROSS_ALIGN_START = 0,
    BUI_CROSS_ALIGN_CENTER,
    BUI_CROSS_ALIGN_END,
    BUI_CROSS_ALIGN_STRETCH
} bui_cross_align_t;

/* Paramètres de mise en page pour un conteneur */
typedef struct {
    bui_layout_direction_t direction;
    bui_main_align_t       main_axis_align;
    bui_cross_align_t      cross_axis_align;
    bui_dp_t               gap;             /* Espace entre chaque enfant (dp) */
    bui_insets_t           padding;         /* Marges intérieures (dp) */
} bui_layout_params_t;

/* Déclaration anticipée du widget */
struct bui_widget;

/* Passe 1 : Mesure récursive. Calcule la taille désirée en fonction des contraintes. */
bui_size_t bui_layout_measure(struct bui_widget* widget, bui_constraints_t constraints);

/* Passe 2 : Placement récursif. Positionne les enfants dans le rectangle alloué. */
void bui_layout_perform(struct bui_widget* widget, bui_rect_t allocated_bounds);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BUI_LAYOUT_H */
