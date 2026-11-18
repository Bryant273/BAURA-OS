/**
 * BAURA OS — BUI SDK & Shell Applications
 * bui_shell_apps.c — Implémentation modulaire des applications du bureau BAURA OS
 */

#include "bui_shell_apps.h"
#include <stdio.h>
#include <string.h>

/* ========================================================================= */
/* 1. APPLICATION SHELL : LE DOCK EN CAPSULES (Split-Dock)                  */
/* ========================================================================= */

static void dock_on_build_ui(bui_app_instance_t* app, bui_widget_t* root) {
    (void)app;
    /* Le dock est un conteneur horizontal (Row) avec padding et effet verre */
    root->layout_params.direction = BUI_LAYOUT_ROW;
    root->layout_params.gap = 8;
    bui_insets_t dock_pad = {6, 12, 6, 12};
    root->layout_params.padding = dock_pad;
    root->corner_radius = 20; /* Style pilule / capsule */
    root->is_glass = true;

    /* Bouton BAURA Start (Logo vectoriel) */
    bui_widget_t* btn_start = bui_create_icon_button("baura_logo", "Démarrer");
    btn_start->bounds.width = 38;
    btn_start->bounds.height = 38;
    bui_widget_add_child(root, btn_start);

    /* Séparateur vertical fin */
    bui_widget_t* sep = bui_create_divider(24, true);
    bui_widget_add_child(root, sep);

    /* Icônes d'applications du Dock */
    const char* dock_icons[] = {"folder", "terminal", "browser", "settings", "store", "trash"};
    const char* dock_labels[] = {"Fichiers", "Terminal", "HOUB Web", "Paramètres", "Boutique", "Corbeille"};

    for (int i = 0; i < 6; i++) {
        bui_widget_t* icon_btn = bui_create_icon_button(dock_icons[i], dock_labels[i]);
        icon_btn->bounds.width = 38;
        icon_btn->bounds.height = 38;
        bui_widget_add_child(root, icon_btn);
    }
}

static const bui_app_desc_t s_dock_desc = {
    .app_id = BUI_APP_ID_DOCK,
    .name = "Split-Dock Système",
    .version = "1.0.0",
    .icon = "dock",
    .role = BUI_APP_ROLE_PANEL,
    .default_width_dp = 380,
    .default_height_dp = 52,
    .min_width_dp = 200,
    .min_height_dp = 44,
    .init = NULL,
    .build_ui = dock_on_build_ui,
    .on_event = NULL,
    .on_command = NULL,
    .on_tick = NULL,
    .destroy = NULL
};

/* ========================================================================= */
/* 2. APPLICATION SHELL : TOP TRAY (Pilule d'état supérieure)                */
/* ========================================================================= */

static void topbar_on_build_ui(bui_app_instance_t* app, bui_widget_t* root) {
    (void)app;
    root->layout_params.direction = BUI_LAYOUT_ROW;
    root->layout_params.gap = 12;
    bui_insets_t pad = {4, 16, 4, 16};
    root->layout_params.padding = pad;
    root->corner_radius = 16;
    root->is_glass = true;

    /* Étiquette Statut / Température */
    bui_widget_t* lbl_stat = bui_create_label("41°C · seL4 OK", BUI_COLOR_RGB(0x10, 0xB9, 0x81));
    bui_widget_add_child(root, lbl_stat);

    /* Séparateur */
    bui_widget_t* sep = bui_create_divider(16, true);
    bui_widget_add_child(root, sep);

    /* Date et Heure */
    bui_widget_t* lbl_clock = bui_create_label("Lun 16 Nov 14:40", BUI_COLOR_RGB(0xF9, 0xFA, 0xFB));
    bui_widget_add_child(root, lbl_clock);
}

static const bui_app_desc_t s_topbar_desc = {
    .app_id = BUI_APP_ID_TOPBAR,
    .name = "Pilule d'État TopBar",
    .version = "1.0.0",
    .icon = "clock",
    .role = BUI_APP_ROLE_PANEL,
    .default_width_dp = 240,
    .default_height_dp = 32,
    .min_width_dp = 120,
    .min_height_dp = 28,
    .init = NULL,
    .build_ui = topbar_on_build_ui,
    .on_event = NULL,
    .on_command = NULL,
    .on_tick = NULL,
    .destroy = NULL
};

/* ========================================================================= */
/* 3. APPLICATION SHELL : MENU BSTART                                        */
/* ========================================================================= */

static void bstart_on_build_ui(bui_app_instance_t* app, bui_widget_t* root) {
    (void)app;
    root->layout_params.direction = BUI_LAYOUT_COLUMN;
    root->layout_params.gap = 10;
    bui_insets_t pad = {12, 12, 12, 12};
    root->layout_params.padding = pad;
    root->corner_radius = 18;
    root->is_glass = true;

    /* Champ de recherche rapide */
    bui_widget_t* search_bar = bui_create_panel(8, false);
    search_bar->bounds.height = 34;
    search_bar->layout_params.direction = BUI_LAYOUT_ROW;
    search_bar->layout_params.gap = 6;
    bui_insets_t s_pad = {4, 8, 4, 8};
    search_bar->layout_params.padding = s_pad;
    
    bui_widget_t* s_lbl = bui_create_label("Rechercher dans BAURA OS...", BUI_COLOR_RGB(0x9C, 0xA3, 0xAF));
    bui_widget_add_child(search_bar, s_lbl);
    bui_widget_add_child(root, search_bar);

    /* Grille d'applications favorites */
    bui_widget_t* row1 = bui_create_row(8, (bui_insets_t){0, 0, 0, 0});
    bui_widget_add_child(row1, bui_create_button("Fichiers", NULL));
    bui_widget_add_child(row1, bui_create_button("Terminal", NULL));
    bui_widget_add_child(row1, bui_create_button("Navigateur", NULL));
    bui_widget_add_child(root, row1);

    bui_widget_t* row2 = bui_create_row(8, (bui_insets_t){0, 0, 0, 0});
    bui_widget_add_child(row2, bui_create_button("Paramètres", NULL));
    bui_widget_add_child(row2, bui_create_button("Moniteur", NULL));
    bui_widget_add_child(row2, bui_create_button("Calculatrice", NULL));
    bui_widget_add_child(root, row2);
}

static const bui_app_desc_t s_bstart_desc = {
    .app_id = BUI_APP_ID_BSTART,
    .name = "Menu BStart",
    .version = "1.0.0",
    .icon = "grid",
    .role = BUI_APP_ROLE_POPUP,
    .default_width_dp = 280,
    .default_height_dp = 240,
    .min_width_dp = 240,
    .min_height_dp = 200,
    .init = NULL,
    .build_ui = bstart_on_build_ui,
    .on_event = NULL,
    .on_command = NULL,
    .on_tick = NULL,
    .destroy = NULL
};

/* ========================================================================= */
/* 4. APPLICATION UTILISATEUR : FICHIERS (File Manager)                      */
/* ========================================================================= */

static void files_on_build_ui(bui_app_instance_t* app, bui_widget_t* root) {
    (void)app;
    /* Agencement en deux colonnes : Sidebar latérale + Vue centrale */
    root->layout_params.direction = BUI_LAYOUT_ROW;
    root->layout_params.gap = 8;
    root->layout_params.padding = (bui_insets_t){4, 4, 4, 4};

    /* Panneau latéral (Sidebar) */
    bui_widget_t* sidebar = bui_create_column(4, (bui_insets_t){8, 8, 8, 8});
    sidebar->bounds.width = 120;
    sidebar->bg_color = BUI_COLOR_ARGB(0x40, 0x11, 0x18, 0x27);
    sidebar->corner_radius = 8;

    bui_widget_add_child(sidebar, bui_create_label("Emplacements", BUI_COLOR_RGB(0x9C, 0xA3, 0xAF)));
    bui_widget_add_child(sidebar, bui_create_button("Documents", NULL));
    bui_widget_add_child(sidebar, bui_create_button("Téléchargements", NULL));
    bui_widget_add_child(sidebar, bui_create_button("Images", NULL));
    bui_widget_add_child(sidebar, bui_create_button("Système PC/C:", NULL));
    bui_widget_add_child(root, sidebar);

    /* Vue principale (Explorateur de dossiers/fichiers) */
    bui_widget_t* main_view = bui_create_column(6, (bui_insets_t){8, 8, 8, 8});
    main_view->flex_grow = 1;

    bui_widget_t* path_bar = bui_create_label("PC/C:/Users/Ahmed/Documents", BUI_COLOR_RGB(0x38, 0xBD, 0xF8));
    bui_widget_add_child(main_view, path_bar);

    bui_widget_t* files_grid = bui_create_row(10, (bui_insets_t){4, 4, 4, 4});
    files_grid->flex_grow = 1;
    bui_widget_add_child(files_grid, bui_create_button("[Dossier] Projets", NULL));
    bui_widget_add_child(files_grid, bui_create_button("[Dossier] Cours", NULL));
    bui_widget_add_child(files_grid, bui_create_button("[Doc] baura_spec.pdf", NULL));
    bui_widget_add_child(main_view, files_grid);

    bui_widget_add_child(root, main_view);
}

static const bui_app_desc_t s_files_desc = {
    .app_id = BUI_APP_ID_FILES,
    .name = "Fichiers",
    .version = "1.0.0",
    .icon = "folder",
    .role = BUI_APP_ROLE_WINDOW,
    .default_width_dp = 440,
    .default_height_dp = 420,
    .min_width_dp = 320,
    .min_height_dp = 240,
    .init = NULL,
    .build_ui = files_on_build_ui,
    .on_event = NULL,
    .on_command = NULL,
    .on_tick = NULL,
    .destroy = NULL
};

/* ========================================================================= */
/* 5. APPLICATION UTILISATEUR : TERMINAL                                     */
/* ========================================================================= */

static void terminal_on_build_ui(bui_app_instance_t* app, bui_widget_t* root) {
    (void)app;
    root->layout_params.direction = BUI_LAYOUT_COLUMN;
    root->layout_params.gap = 4;
    root->layout_params.padding = (bui_insets_t){8, 12, 8, 12};
    root->bg_color = BUI_COLOR_ARGB(0xE0, 0x05, 0x08, 0x11); /* Noir profond console */

    /* Lignes de texte du terminal */
    bui_widget_add_child(root, bui_create_label("BAURA OS Shell v2.4 (x86_64 pc99 seL4)", BUI_COLOR_RGB(0x9C, 0xA3, 0xAF)));
    bui_widget_add_child(root, bui_create_label("Kernel: seL4 microkernel 16.0.0-dev | Driver: PS/2 Mouse active", BUI_COLOR_RGB(0x10, 0xB9, 0x81)));
    bui_widget_add_child(root, bui_create_label("Memory: 2048 MB RAM | Framebuffer: 1024x768x32 ARGB", BUI_COLOR_RGB(0x9C, 0xA3, 0xAF)));
    bui_widget_add_child(root, bui_create_divider(2, false));

    /* Invite de commande */
    bui_widget_t* prompt_row = bui_create_row(6, (bui_insets_t){4, 0, 4, 0});
    bui_widget_add_child(prompt_row, bui_create_label("baura@pc99:~$", BUI_COLOR_RGB(0x38, 0xBD, 0xF8)));
    bui_widget_add_child(prompt_row, bui_create_label("ls -l /system/bin", BUI_COLOR_RGB(0xF9, 0xFA, 0xFB)));
    
    /* Curseur vert */
    bui_widget_t* cursor = bui_widget_create(BUI_WIDGET_CUSTOM, 99);
    cursor->bounds.width = 8;
    cursor->bounds.height = 16;
    cursor->bg_color = BUI_COLOR_RGB(0x10, 0xB9, 0x81);
    bui_widget_add_child(prompt_row, cursor);

    bui_widget_add_child(root, prompt_row);
}

static const bui_app_desc_t s_terminal_desc = {
    .app_id = BUI_APP_ID_TERMINAL,
    .name = "Terminal",
    .version = "1.0.0",
    .icon = "terminal",
    .role = BUI_APP_ROLE_WINDOW,
    .default_width_dp = 456,
    .default_height_dp = 420,
    .min_width_dp = 300,
    .min_height_dp = 200,
    .init = NULL,
    .build_ui = terminal_on_build_ui,
    .on_event = NULL,
    .on_command = NULL,
    .on_tick = NULL,
    .destroy = NULL
};

/* ========================================================================= */
/* REGISTRE CENTRAL                                                          */
/* ========================================================================= */

void bui_shell_apps_register_all(void) {
    bui_app_register(&s_dock_desc);
    bui_app_register(&s_topbar_desc);
    bui_app_register(&s_bstart_desc);
    bui_app_register(&s_files_desc);
    bui_app_register(&s_terminal_desc);
}

const bui_app_desc_t* bui_get_dock_app_desc(void) { return &s_dock_desc; }
const bui_app_desc_t* bui_get_topbar_app_desc(void) { return &s_topbar_desc; }
const bui_app_desc_t* bui_get_bstart_app_desc(void) { return &s_bstart_desc; }
const bui_app_desc_t* bui_get_files_app_desc(void) { return &s_files_desc; }
const bui_app_desc_t* bui_get_terminal_app_desc(void) { return &s_terminal_desc; }
