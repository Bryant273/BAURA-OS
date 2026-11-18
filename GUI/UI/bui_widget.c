/**
 * BAURA OS — BUI SDK
 * bui_widget.c — Arbre de widgets, gestionnaires de style, constructeurs et routage
 */

#include "bui_widget.h"
#include <stdlib.h>
#include <string.h>

static bui_theme_t s_default_theme = {
    .colors = {
        .accent_1       = BUI_COLOR_RGB(0x1D, 0x4E, 0xD8),
        .accent_2       = BUI_COLOR_RGB(0x38, 0xBD, 0xF8),
        .bg_night       = BUI_COLOR_RGB(0x0B, 0x0F, 0x19),
        .glass_bg       = BUI_COLOR_ARGB(0xA6, 0x10, 0x18, 0x27),
        .glass_border   = BUI_COLOR_ARGB(0x26, 0xFF, 0xFF, 0xFF),
        .hairline       = BUI_COLOR_ARGB(0x1A, 0xFF, 0xFF, 0xFF),
        .fg_primary     = BUI_COLOR_RGB(0xF9, 0xFA, 0xFB),
        .fg_secondary   = BUI_COLOR_RGB(0x9C, 0xA3, 0xAF),
        .fg_muted       = BUI_COLOR_RGB(0x4B, 0x55, 0x63),
        .surface_field  = BUI_COLOR_ARGB(0x80, 0x1F, 0x29, 0x37),
        .hover_overlay  = BUI_COLOR_ARGB(0x14, 0xFF, 0xFF, 0xFF),
        .press_overlay  = BUI_COLOR_ARGB(0x24, 0xFF, 0xFF, 0xFF),
        .error_color    = BUI_COLOR_RGB(0xEF, 0x44, 0x44),
        .success_color  = BUI_COLOR_RGB(0x10, 0xB9, 0x81),
    },
    .radii = {
        .radius_xs   = 4,
        .radius_sm   = 6,
        .radius_md   = 10,
        .radius_lg   = 14,
        .radius_xl   = 20,
        .radius_full = 999,
    },
    .spacing = {
        .space_1 = 4,
        .space_2 = 8,
        .space_3 = 12,
        .space_4 = 16,
        .space_5 = 24,
        .space_6 = 32,
    },
    .scale_factor_pct = 100
};

const bui_theme_t* bui_theme_default(void) {
    return &s_default_theme;
}

bui_rect_t bui_rect_make(bui_dp_t x, bui_dp_t y, bui_dp_t w, bui_dp_t h) {
    bui_rect_t r = {x, y, w, h};
    return r;
}

bool bui_rect_contains(const bui_rect_t* rect, bui_point_t pt) {
    if (!rect) return false;
    return (pt.x >= rect->x && pt.x < (rect->x + rect->width) &&
            pt.y >= rect->y && pt.y < (rect->y + rect->height));
}

bui_widget_t* bui_widget_create(bui_widget_type_t type, uint32_t id) {
    bui_widget_t* w = (bui_widget_t*)calloc(1, sizeof(bui_widget_t));
    if (!w) return NULL;

    w->id = id;
    w->type = type;
    w->visible = true;
    w->interactive = true;
    w->bg_color = s_default_theme.colors.surface_field;
    w->fg_color = s_default_theme.colors.fg_primary;
    w->corner_radius = s_default_theme.radii.radius_md;

    return w;
}

void bui_widget_destroy(bui_widget_t* widget) {
    if (!widget) return;

    bui_widget_t* child = widget->first_child;
    while (child) {
        bui_widget_t* next = child->next_sibling;
        bui_widget_destroy(child);
        child = next;
    }

    free(widget);
}

void bui_widget_add_child(bui_widget_t* parent, bui_widget_t* child) {
    if (!parent || !child) return;

    child->parent = parent;
    child->next_sibling = NULL;
    child->prev_sibling = parent->last_child;

    if (parent->last_child) {
        parent->last_child->next_sibling = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;
    parent->child_count++;
}

void bui_widget_remove_child(bui_widget_t* parent, bui_widget_t* child) {
    if (!parent || !child || child->parent != parent) return;

    if (child->prev_sibling) {
        child->prev_sibling->next_sibling = child->next_sibling;
    } else {
        parent->first_child = child->next_sibling;
    }

    if (child->next_sibling) {
        child->next_sibling->prev_sibling = child->prev_sibling;
    } else {
        parent->last_child = child->prev_sibling;
    }

    child->parent = NULL;
    child->prev_sibling = NULL;
    child->next_sibling = NULL;
    parent->child_count--;
}

bui_widget_t* bui_create_row(bui_dp_t gap, bui_insets_t padding) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_CONTAINER, 0);
    if (!w) return NULL;
    w->layout_params.direction = BUI_LAYOUT_ROW;
    w->layout_params.gap = gap;
    w->layout_params.padding = padding;
    w->layout_params.cross_axis_align = BUI_CROSS_ALIGN_CENTER;
    return w;
}

bui_widget_t* bui_create_column(bui_dp_t gap, bui_insets_t padding) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_CONTAINER, 0);
    if (!w) return NULL;
    w->layout_params.direction = BUI_LAYOUT_COLUMN;
    w->layout_params.gap = gap;
    w->layout_params.padding = padding;
    w->layout_params.cross_axis_align = BUI_CROSS_ALIGN_STRETCH;
    return w;
}

bui_widget_t* bui_create_panel(bui_dp_t corner_radius, bool is_glass) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_PANEL, 0);
    if (!w) return NULL;
    w->corner_radius = corner_radius;
    w->is_glass = is_glass;
    w->bg_color = is_glass ? s_default_theme.colors.glass_bg : s_default_theme.colors.surface_field;
    return w;
}

bui_widget_t* bui_create_label(const char* text, bui_color_t color) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_LABEL, 0);
    if (!w) return NULL;
    if (text) {
        strncpy(w->text, text, sizeof(w->text) - 1);
    }
    w->fg_color = color ? color : s_default_theme.colors.fg_primary;
    w->interactive = false;
    return w;
}

bui_widget_t* bui_create_button(const char* label, void (*on_click)(bui_widget_t*, const bui_event_t*, void*)) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_BUTTON, 0);
    if (!w) return NULL;
    if (label) {
        strncpy(w->text, label, sizeof(w->text) - 1);
    }
    w->bg_color = s_default_theme.colors.accent_1;
    w->fg_color = s_default_theme.colors.fg_primary;
    w->corner_radius = s_default_theme.radii.radius_sm;
    w->on_click = on_click;
    return w;
}

bui_widget_t* bui_create_icon_button(const char* icon_name, const char* tooltip) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_ICON_BUTTON, 0);
    if (!w) return NULL;
    if (icon_name) {
        strncpy(w->icon, icon_name, sizeof(w->icon) - 1);
    }
    if (tooltip) {
        strncpy(w->text, tooltip, sizeof(w->text) - 1);
    }
    w->corner_radius = s_default_theme.radii.radius_md;
    w->bg_color = s_default_theme.colors.glass_bg;
    return w;
}

bui_widget_t* bui_create_divider(bui_dp_t length, bool vertical) {
    bui_widget_t* w = bui_widget_create(BUI_WIDGET_DIVIDER, 0);
    if (!w) return NULL;
    w->interactive = false;
    w->bg_color = s_default_theme.colors.hairline;
    if (vertical) {
        w->bounds.width = 1;
        w->bounds.height = length;
    } else {
        w->bounds.width = length;
        w->bounds.height = 1;
    }
    return w;
}

bui_widget_t* bui_create_window_frame(const char* title, bui_dp_t width_dp, bui_dp_t height_dp) {
    bui_widget_t* frame = bui_widget_create(BUI_WIDGET_WINDOW_FRAME, 0);
    if (!frame) return NULL;

    frame->bounds.width = width_dp;
    frame->bounds.height = height_dp;
    frame->corner_radius = s_default_theme.radii.radius_lg;
    frame->is_glass = true;
    frame->layout_params.direction = BUI_LAYOUT_COLUMN;
    frame->layout_params.gap = 0;

    /* Barre de titre (Row, h = 36 dp) */
    bui_insets_t tb_pad = {6, 12, 6, 12};
    bui_widget_t* title_bar = bui_create_row(8, tb_pad);
    title_bar->bounds.height = 36;
    title_bar->bg_color = BUI_COLOR_ARGB(0x40, 0x11, 0x18, 0x27);

    /* Boutons ronds de contrôle (Rouge, Jaune, Vert style mac/baura) */
    bui_widget_t* close_btn = bui_widget_create(BUI_WIDGET_BUTTON, 1);
    close_btn->bounds.width = 12;
    close_btn->bounds.height = 12;
    close_btn->corner_radius = 6;
    close_btn->bg_color = BUI_COLOR_RGB(0xEF, 0x44, 0x44); /* Rouge close */
    bui_widget_add_child(title_bar, close_btn);

    bui_widget_t* min_btn = bui_widget_create(BUI_WIDGET_BUTTON, 2);
    min_btn->bounds.width = 12;
    min_btn->bounds.height = 12;
    min_btn->corner_radius = 6;
    min_btn->bg_color = BUI_COLOR_RGB(0xF5, 0x9E, 0x0B); /* Jaune min */
    bui_widget_add_child(title_bar, min_btn);

    bui_widget_t* max_btn = bui_widget_create(BUI_WIDGET_BUTTON, 3);
    max_btn->bounds.width = 12;
    max_btn->bounds.height = 12;
    max_btn->corner_radius = 6;
    max_btn->bg_color = BUI_COLOR_RGB(0x10, 0xB9, 0x81); /* Vert max */
    bui_widget_add_child(title_bar, max_btn);

    /* Titre centré */
    bui_widget_t* title_label = bui_create_label(title, s_default_theme.colors.fg_primary);
    title_label->flex_grow = 1;
    bui_widget_add_child(title_bar, title_label);

    bui_widget_add_child(frame, title_bar);

    /* Zone de contenu principale (Content container avec flex_grow = 1) */
    bui_insets_t content_pad = {8, 8, 8, 8};
    bui_widget_t* content_area = bui_create_column(6, content_pad);
    content_area->flex_grow = 1;
    bui_widget_add_child(frame, content_area);

    return frame;
}

bui_widget_t* bui_widget_hit_test(bui_widget_t* root, bui_point_t pt) {
    if (!root || !root->visible) return NULL;
    if (!bui_rect_contains(&root->bounds, pt)) return NULL;

    /* Recherche de bas en haut (enfants les plus profonds d'abord) */
    bui_widget_t* child = root->last_child;
    while (child) {
        bui_widget_t* hit = bui_widget_hit_test(child, pt);
        if (hit) return hit;
        child = child->prev_sibling;
    }

    if (root->interactive) return root;
    return NULL;
}

bool bui_widget_dispatch_event(bui_widget_t* root, const bui_event_t* evt) {
    if (!root || !evt) return false;

    bui_widget_t* target = bui_widget_hit_test(root, evt->pos);
    if (!target) return false;

    if (evt->type == BUI_EVENT_MOUSE_CLICK && target->on_click) {
        target->on_click(target, evt, target->user_data);
        return true;
    }

    return true;
}
