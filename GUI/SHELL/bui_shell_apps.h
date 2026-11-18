/**
 * BAURA OS — BUI SDK & Shell Applications
 * bui_shell_apps.h — Composants d'interface système modélisés comme de véritables applications BUI
 * 
 * Conforme à la directive d'Ahmed Silué :
 * "Aussi évite de coder des applis en dur dès le départ... Lorsqu'on aura un truc solide
 *  on va coder les applis... Aussi on peut considérer les éléments de l'interface comme
 *  des applis pour une meilleure prise en main et maintenance, on doit donc définir un SDK
 *  et si ça passe bien c'est ça qui va devenir notre structure d'appli pour notre os"
 */

#ifndef BAURA_BUI_SHELL_APPS_H
#define BAURA_BUI_SHELL_APPS_H

#include "../UI/bui_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Identifiants des applications système et utilisateur de référence */
#define BUI_APP_ID_DOCK     "org.baura.shell.dock"
#define BUI_APP_ID_TOPBAR   "org.baura.shell.topbar"
#define BUI_APP_ID_BSTART   "org.baura.shell.bstart"
#define BUI_APP_ID_FILES    "org.baura.apps.files"
#define BUI_APP_ID_TERMINAL "org.baura.apps.terminal"
#define BUI_APP_ID_MONITOR  "org.baura.apps.monitor"

/* Enregistre l'ensemble des descripteurs d'applications du système */
void bui_shell_apps_register_all(void);

/* Accès direct aux descripteurs */
const bui_app_desc_t* bui_get_dock_app_desc(void);
const bui_app_desc_t* bui_get_topbar_app_desc(void);
const bui_app_desc_t* bui_get_bstart_app_desc(void);
const bui_app_desc_t* bui_get_files_app_desc(void);
const bui_app_desc_t* bui_get_terminal_app_desc(void);

#ifdef __cplusplus
}
#endif

#endif /* BAURA_BUI_SHELL_APPS_H */
