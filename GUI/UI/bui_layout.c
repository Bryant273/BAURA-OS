/**
 * BAURA OS — BUI SDK
 * bui_layout.c — Implémentation du moteur de mise en page récursif en 2 passes
 */

#include "bui_layout.h"
#include "bui_widget.h"
#include <string.h>

static inline bui_dp_t clamp_dp(bui_dp_t val, bui_dp_t min_v, bui_dp_t max_v) {
    if (val < min_v) return min_v;
    if (max_v > 0 && val > max_v) return max_v;
    return val;
}

/* Passe 1 : Mesure récursive */
bui_size_t bui_layout_measure(bui_widget_t* w, bui_constraints_t constraints) {
    if (!w || !w->visible) {
        bui_size_t zero = {0, 0};
        return zero;
    }

    bui_size_t measured = {0, 0};

    /* Si c'est une feuille (Label, Bouton, Divider) */
    if (!w->first_child) {
        if (w->type == BUI_WIDGET_LABEL || w->type == BUI_WIDGET_BUTTON || w->type == BUI_WIDGET_ICON_BUTTON) {
            /* Mesure basée sur le texte / icône : 8 dp par caractère + padding */
            size_t text_len = strlen(w->text);
            bui_dp_t content_w = (bui_dp_t)(text_len * 8);
            if (w->type == BUI_WIDGET_BUTTON) content_w += 24; /* Padding horizontal */
            if (w->type == BUI_WIDGET_ICON_BUTTON) content_w = 36;
            
            measured.width = clamp_dp(content_w, constraints.min_width, constraints.max_width);
            measured.height = clamp_dp(32, constraints.min_height, constraints.max_height);
        } else if (w->type == BUI_WIDGET_DIVIDER) {
            measured.width = (w->layout_params.direction == BUI_LAYOUT_ROW) ? 1 : constraints.max_width;
            measured.height = (w->layout_params.direction == BUI_LAYOUT_ROW) ? constraints.max_height : 1;
        } else {
            measured.width = clamp_dp(w->bounds.width ? w->bounds.width : 40, constraints.min_width, constraints.max_width);
            measured.height = clamp_dp(w->bounds.height ? w->bounds.height : 40, constraints.min_height, constraints.max_height);
        }
        w->measured_size = measured;
        return measured;
    }

    /* Conteneur composé : Mesure des enfants */
    bui_dp_t total_main = 0;
    bui_dp_t max_cross = 0;
    bui_dp_t gap = w->layout_params.gap;
    bui_insets_t pad = w->layout_params.padding;

    bui_constraints_t child_constraints;
    child_constraints.min_width = 0;
    child_constraints.max_width = (constraints.max_width > pad.left + pad.right) ? 
                                   (constraints.max_width - (pad.left + pad.right)) : 0;
    child_constraints.min_height = 0;
    child_constraints.max_height = (constraints.max_height > pad.top + pad.bottom) ? 
                                    (constraints.max_height - (pad.top + pad.bottom)) : 0;

    bui_widget_t* child = w->first_child;
    bool is_first = true;

    while (child) {
        if (child->visible) {
            bui_size_t c_size = bui_layout_measure(child, child_constraints);
            if (w->layout_params.direction == BUI_LAYOUT_ROW) {
                total_main += c_size.width;
                if (!is_first) total_main += gap;
                if (c_size.height > max_cross) max_cross = c_size.height;
            } else if (w->layout_params.direction == BUI_LAYOUT_COLUMN) {
                total_main += c_size.height;
                if (!is_first) total_main += gap;
                if (c_size.width > max_cross) max_cross = c_size.width;
            } else { /* BUI_LAYOUT_STACK */
                if (c_size.width > max_cross) max_cross = c_size.width;
                if (c_size.height > total_main) total_main = c_size.height;
            }
            is_first = false;
        }
        child = child->next_sibling;
    }

    if (w->layout_params.direction == BUI_LAYOUT_ROW) {
        measured.width = total_main + pad.left + pad.right;
        measured.height = max_cross + pad.top + pad.bottom;
    } else if (w->layout_params.direction == BUI_LAYOUT_COLUMN) {
        measured.width = max_cross + pad.left + pad.right;
        measured.height = total_main + pad.top + pad.bottom;
    } else {
        measured.width = max_cross + pad.left + pad.right;
        measured.height = total_main + pad.top + pad.bottom;
    }

    measured.width = clamp_dp(measured.width, constraints.min_width, constraints.max_width);
    measured.height = clamp_dp(measured.height, constraints.min_height, constraints.max_height);

    w->measured_size = measured;
    return measured;
}

/* Passe 2 : Placement récursif */
void bui_layout_perform(bui_widget_t* w, bui_rect_t allocated_bounds) {
    if (!w || !w->visible) return;

    w->bounds = allocated_bounds;
    if (!w->first_child) return;

    bui_insets_t pad = w->layout_params.padding;
    bui_dp_t inner_x = allocated_bounds.x + pad.left;
    bui_dp_t inner_y = allocated_bounds.y + pad.top;
    bui_dp_t inner_w = (allocated_bounds.width > pad.left + pad.right) ? 
                        (allocated_bounds.width - (pad.left + pad.right)) : 0;
    bui_dp_t inner_h = (allocated_bounds.height > pad.top + pad.bottom) ? 
                        (allocated_bounds.height - (pad.top + pad.bottom)) : 0;

    bui_dp_t gap = w->layout_params.gap;

    /* Calcul du flex grow */
    bui_dp_t total_flex = 0;
    bui_dp_t non_flex_main = 0;
    uint32_t visible_child_count = 0;

    bui_widget_t* child = w->first_child;
    while (child) {
        if (child->visible) {
            visible_child_count++;
            if (child->flex_grow > 0) {
                total_flex += child->flex_grow;
            } else {
                non_flex_main += (w->layout_params.direction == BUI_LAYOUT_ROW) ? 
                                 child->measured_size.width : child->measured_size.height;
            }
        }
        child = child->next_sibling;
    }

    bui_dp_t total_gaps = (visible_child_count > 1) ? (bui_dp_t)((visible_child_count - 1) * gap) : 0;
    bui_dp_t available_for_flex = 0;

    if (w->layout_params.direction == BUI_LAYOUT_ROW) {
        bui_dp_t used = non_flex_main + total_gaps;
        if (inner_w > used) available_for_flex = inner_w - used;
    } else if (w->layout_params.direction == BUI_LAYOUT_COLUMN) {
        bui_dp_t used = non_flex_main + total_gaps;
        if (inner_h > used) available_for_flex = inner_h - used;
    }

    /* Placement de chaque enfant */
    bui_dp_t cur_main = (w->layout_params.direction == BUI_LAYOUT_ROW) ? inner_x : inner_y;

    child = w->first_child;
    while (child) {
        if (child->visible) {
            bui_rect_t child_rect;

            if (w->layout_params.direction == BUI_LAYOUT_ROW) {
                bui_dp_t c_width = child->measured_size.width;
                if (child->flex_grow > 0 && total_flex > 0) {
                    c_width = (available_for_flex * child->flex_grow) / total_flex;
                }

                child_rect.x = cur_main;
                child_rect.y = inner_y;
                child_rect.width = c_width;
                child_rect.height = (w->layout_params.cross_axis_align == BUI_CROSS_ALIGN_STRETCH) ? 
                                     inner_h : child->measured_size.height;

                cur_main += c_width + gap;
            } else if (w->layout_params.direction == BUI_LAYOUT_COLUMN) {
                bui_dp_t c_height = child->measured_size.height;
                if (child->flex_grow > 0 && total_flex > 0) {
                    c_height = (available_for_flex * child->flex_grow) / total_flex;
                }

                child_rect.x = inner_x;
                child_rect.y = cur_main;
                child_rect.width = (w->layout_params.cross_axis_align == BUI_CROSS_ALIGN_STRETCH) ? 
                                    inner_w : child->measured_size.width;
                child_rect.height = c_height;

                cur_main += c_height + gap;
            } else { /* STACK */
                child_rect.x = inner_x;
                child_rect.y = inner_y;
                child_rect.width = inner_w;
                child_rect.height = inner_h;
            }

            bui_layout_perform(child, child_rect);
        }
        child = child->next_sibling;
    }
}
