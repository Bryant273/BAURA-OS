/* BAURA OS — baura_ui_services.h : interface C du backend UI (Rust, crate baura-ffi → libbaura_ui_services.a).
 * Écrit à la main et vérifié par tests/test_backend.cpp (le C++ qui l'utilise est compilé -Wall -Wextra -Werror).
 * Convention : chaînes UTF-8 copiées dans un tampon appelant (buf, cap) ; la valeur de retour est la longueur NÉCESSAIRE (sans NUL). */
#ifndef BAURA_UI_SERVICES_H
#define BAURA_UI_SERVICES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct BauraServices BauraServices;   /* objet opaque */
typedef struct { float cpu_pct; uint64_t rss_kib; uint32_t processes; uint32_t threads; uint32_t impact; /* 0 faible, 1 moyen, 2 élevé */ } BauraGroup;
typedef struct { uint64_t t_ms; float cpu_total_pct; uint64_t mem_total_kib, mem_used_kib, swap_total_kib, swap_used_kib, net_rx_bps, net_tx_bps, disk_read_bps, disk_write_bps; float battery_pct /* -1 = absent */, temp_c /* -1 = inconnue */; uint32_t cores; } BauraSnapshot;
typedef struct { uint8_t pinned, running, focused, attention; uint32_t window_count, badge; } BauraTaskbarItem;

/* cycle de vie — source : 0 = synthétique (déterministe), 1 = /proc Linux (sinon synthétique) */
BauraServices* baura_new(int source, uint64_t seed);
void baura_free(BauraServices*);
uint32_t baura_version(void);
/* préférences — disposition : 0 barre coupée en deux (DÉFAUT) · 1 pastille flottante · 2 pleine largeur ; retour des set : 1 changé, 0 inchangé, -1 valeur invalide */
uint64_t baura_prefs_version(BauraServices*);
uint32_t baura_prefs_get_layout(BauraServices*);      int baura_prefs_set_layout(BauraServices*, uint32_t);
uint32_t baura_prefs_get_theme(BauraServices*);       int baura_prefs_set_theme(BauraServices*, uint32_t);       /* auto, clair, sombre, hc-clair, hc-sombre */
uint32_t baura_prefs_get_icon_theme(BauraServices*);  int baura_prefs_set_icon_theme(BauraServices*, uint32_t);  /* 0 BAURA, 1 d'origine */
uint32_t baura_prefs_get_density(BauraServices*);     int baura_prefs_set_density(BauraServices*, uint32_t);     /* 0 confortable, 1 compacte */
int baura_prefs_is_rtl(BauraServices*);
int baura_prefs_set_language(BauraServices*, const char* lang);   size_t baura_prefs_get_language(BauraServices*, char* buf, size_t cap);
uint32_t baura_prefs_text_scale(BauraServices*);      int baura_prefs_set_text_scale(BauraServices*, uint32_t pct);
size_t baura_prefs_serialize(BauraServices*, char* buf, size_t cap);   int baura_prefs_load(BauraServices*, const char* text); /* → nb de lignes invalides */
/* moniteur */
void baura_monitor_tick(BauraServices*, uint64_t now_ms);
void baura_monitor_snapshot(BauraServices*, BauraSnapshot* out);
float baura_monitor_core(BauraServices*, uint32_t i);
size_t baura_monitor_history(BauraServices*, uint32_t kind /* 0 cpu%, 1 mém%, 2 réseau B/s, 3 disque B/s */, float* out, size_t cap);
size_t baura_monitor_group_count(BauraServices*);
int baura_monitor_group(BauraServices*, size_t i, BauraGroup* out, char* name, size_t name_cap, char* icon, size_t icon_cap);
int baura_monitor_terminate(BauraServices*, const char* group_name);   /* 0 terminé · 1 protégé · 2 introuvable */
/* barre des tâches, recherche, icônes */
void baura_window_open(BauraServices*, uint32_t id, const char* app_id, const char* title);
void baura_window_close(BauraServices*, uint32_t id);
void baura_window_set_badge(BauraServices*, uint32_t id, uint32_t badge);
size_t baura_taskbar_refresh(BauraServices*);
int baura_taskbar_item(BauraServices*, size_t i, BauraTaskbarItem* out, char* app_id, size_t id_cap, char* icon, size_t icon_cap);
int baura_taskbar_click(BauraServices*, size_t i, uint32_t* window);   /* 0 lancer · 1 focaliser · 2 réduire · 3 restaurer · 4 parcourir */
size_t baura_search(BauraServices*, const char* query, size_t limit);
int baura_search_hit(BauraServices*, size_t i, char* id, size_t id_cap, char* title, size_t title_cap, char* icon, size_t icon_cap);   /* → 0 appli · 1 réglage · 2 fichier */
size_t baura_icon_resolve(const char* app_id, const char* desktop_categories, char* buf, size_t cap);
/* notifications */
int baura_notify(BauraServices*, const char* app, const char* title, const char* body, uint64_t t_ms, int urgent);   /* → 1 si une bulle doit s'afficher */
size_t baura_notif_unread(BauraServices*);  size_t baura_notif_count(BauraServices*);
void baura_notif_set_dnd(BauraServices*, int on);  void baura_notif_mark_read(BauraServices*);  void baura_notif_clear(BauraServices*);
/* traductions (langue courante des préférences) */
size_t baura_tr(BauraServices*, const char* key, char* buf, size_t cap);
size_t baura_tr_n(BauraServices*, const char* key, uint64_t n, char* buf, size_t cap);
size_t baura_catalog_count(BauraServices*);
#ifdef __cplusplus
}
#endif
#endif
