/**
 * BAURA OS — BUI SDK
 * bui_app.c — Registre et Gestionnaire du Cycle de Vie des Applications BUI
 */

#include "bui_app.h"
#include <stdlib.h>
#include <string.h>

#define MAX_REGISTERED_APPS 64

static const bui_app_desc_t* s_registered_apps[MAX_REGISTERED_APPS];
static uint32_t              s_registered_count = 0;
static bui_app_instance_t*   s_running_instances = NULL;
static uint32_t              s_next_instance_id = 1;

bool bui_app_register(const bui_app_desc_t* desc) {
    if (!desc || !desc->app_id) return false;
    if (s_registered_count >= MAX_REGISTERED_APPS) return false;

    /* Vérifier les doublons */
    for (uint32_t i = 0; i < s_registered_count; i++) {
        if (strcmp(s_registered_apps[i]->app_id, desc->app_id) == 0) {
            return false; /* Déjà enregistré */
        }
    }

    s_registered_apps[s_registered_count++] = desc;
    return true;
}

const bui_app_desc_t* bui_app_find_desc(const char* app_id) {
    if (!app_id) return NULL;
    for (uint32_t i = 0; i < s_registered_count; i++) {
        if (strcmp(s_registered_apps[i]->app_id, app_id) == 0) {
            return s_registered_apps[i];
        }
    }
    return NULL;
}

uint32_t bui_app_get_registered_count(void) {
    return s_registered_count;
}

const bui_app_desc_t* bui_app_get_desc_by_index(uint32_t index) {
    if (index >= s_registered_count) return NULL;
    return s_registered_apps[index];
}

bui_app_instance_t* bui_app_launch(const char* app_id) {
    const bui_app_desc_t* desc = bui_app_find_desc(app_id);
    if (!desc) return NULL;

    bui_app_instance_t* inst = (bui_app_instance_t*)calloc(1, sizeof(bui_app_instance_t));
    if (!inst) return NULL;

    inst->desc = desc;
    inst->instance_id = s_next_instance_id++;
    inst->is_running = true;
    inst->is_visible = true;
    inst->is_focused = true;

    /* Initialisation personnalisée via hook SDK */
    if (desc->init) {
        if (!desc->init(inst)) {
            free(inst);
            return NULL;
        }
    }

    /* Construction de l'arborescence UI */
    if (desc->role == BUI_APP_ROLE_WINDOW) {
        inst->window_frame = bui_create_window_frame(desc->name, desc->default_width_dp, desc->default_height_dp);
        inst->root_widget = inst->window_frame;
        
        /* Récupérer la zone de contenu (2ème enfant du frame) */
        if (inst->window_frame && inst->window_frame->child_count >= 2) {
            bui_widget_t* content = inst->window_frame->last_child;
            if (desc->build_ui) {
                desc->build_ui(inst, content);
            }
        }
    } else {
        /* Panneau, Split-Dock ou Menu volant */
        inst->root_widget = bui_create_panel(16, true);
        if (desc->build_ui) {
            desc->build_ui(inst, inst->root_widget);
        }
    }

    /* Insertion en tête de la liste des instances */
    inst->next = s_running_instances;
    s_running_instances = inst;

    return inst;
}

void bui_app_terminate(bui_app_instance_t* instance) {
    if (!instance) return;

    /* Retrait de la liste */
    bui_app_instance_t** curr = &s_running_instances;
    while (*curr) {
        if (*curr == instance) {
            *curr = instance->next;
            break;
        }
        curr = &(*curr)->next;
    }

    /* Appel du hook de destruction SDK */
    if (instance->desc && instance->desc->destroy) {
        instance->desc->destroy(instance);
    }

    /* Destruction de l'arbre UI */
    if (instance->root_widget) {
        bui_widget_destroy(instance->root_widget);
        instance->root_widget = NULL;
    }

    free(instance);
}

void bui_app_broadcast_tick(uint32_t delta_ms) {
    bui_app_instance_t* inst = s_running_instances;
    while (inst) {
        if (inst->is_running && inst->desc && inst->desc->on_tick) {
            inst->desc->on_tick(inst, delta_ms);
        }
        inst = inst->next;
    }
}

void bui_app_dispatch_event_to_focused(const bui_event_t* event) {
    if (!event) return;

    bui_app_instance_t* inst = s_running_instances;
    while (inst) {
        if (inst->is_running && inst->is_focused) {
            if (inst->desc && inst->desc->on_event) {
                inst->desc->on_event(inst, event);
            }
            if (inst->root_widget) {
                bui_widget_dispatch_event(inst->root_widget, event);
            }
            break;
        }
        inst = inst->next;
    }
}
