//! Interface C (ABI stable) du backend UI : un seul objet opaque `BauraServices` + fonctions `extern "C"`.
//! Tout est « snapshot + commandes » : le C++ demande une vue, la dessine, puis envoie des commandes (clic, réglage, terminaison).
//! Chaînes : UTF-8, copiées dans un tampon fourni par l'appelant (`buf`, `cap`) ; retour = longueur nécessaire (sans NUL).
#![allow(clippy::missing_safety_doc)]
use baura_i18n::Catalog; use baura_prefs::*; use baura_shell::*; use baura_sysmon::*;
use std::ffi::{c_char, CStr}; use std::os::raw::c_int;

pub struct BauraServices { prefs: Prefs, monitor: Monitor, windows: Vec<Window>, catalog: Vec<App>, items: Vec<Item>, notifs: Notifications, i18n: Catalog, recent: Vec<String>, groups: Vec<AppGroup>, tb: Vec<TaskbarItem>, hits: Vec<Hit> }
#[repr(C)] #[derive(Clone, Copy, Default)] pub struct BauraGroup { pub cpu_pct: f32, pub rss_kib: u64, pub processes: u32, pub threads: u32, pub impact: u32 }
#[repr(C)] #[derive(Clone, Copy, Default)] pub struct BauraSnapshot { pub t_ms: u64, pub cpu_total_pct: f32, pub mem_total_kib: u64, pub mem_used_kib: u64, pub swap_total_kib: u64, pub swap_used_kib: u64, pub net_rx_bps: u64, pub net_tx_bps: u64, pub disk_read_bps: u64, pub disk_write_bps: u64, pub battery_pct: f32, pub temp_c: f32, pub cores: u32 }
#[repr(C)] #[derive(Clone, Copy, Default)] pub struct BauraTaskbarItem { pub pinned: u8, pub running: u8, pub focused: u8, pub attention: u8, pub window_count: u32, pub badge: u32 }

unsafe fn cstr<'a>(p: *const c_char) -> &'a str { if p.is_null() { "" } else { CStr::from_ptr(p).to_str().unwrap_or("") } }
unsafe fn put(s: &str, buf: *mut c_char, cap: usize) -> usize { if !buf.is_null() && cap > 0 { let n = s.len().min(cap - 1); std::ptr::copy_nonoverlapping(s.as_ptr(), buf as *mut u8, n); *buf.add(n) = 0; } s.len() }
fn h<'a>(p: *mut BauraServices) -> &'a mut BauraServices { assert!(!p.is_null()); unsafe { &mut *p } }

/// `source` : 0 = synthétique (démo/tests), 1 = /proc Linux (si disponible, sinon synthétique).
#[no_mangle] pub extern "C" fn baura_new(source: c_int, seed: u64) -> *mut BauraServices {
    let src: Box<dyn Source> = if source == 1 && LinuxProc::available() { Box::new(LinuxProc::new()) } else { Box::new(Synthetic::new(seed)) };
    let catalog = default_catalog(); let items = items_from_catalog(&catalog);
    Box::into_raw(Box::new(BauraServices { prefs: Prefs::default(), monitor: Monitor::new(src, 120), windows: vec![], catalog, items, notifs: Notifications::default(), i18n: baura_i18n::builtin(), recent: vec![], groups: vec![], tb: vec![], hits: vec![] }))
}
#[no_mangle] pub unsafe extern "C" fn baura_free(p: *mut BauraServices) { if !p.is_null() { drop(Box::from_raw(p)); } }
#[no_mangle] pub extern "C" fn baura_version() -> u32 { 0x0001_0000 }

// ---- préférences
#[no_mangle] pub extern "C" fn baura_prefs_version(p: *mut BauraServices) -> u64 { h(p).prefs.version }
#[no_mangle] pub extern "C" fn baura_prefs_get_layout(p: *mut BauraServices) -> u32 { h(p).prefs.layout.index() }
#[no_mangle] pub extern "C" fn baura_prefs_set_layout(p: *mut BauraServices, v: u32) -> c_int { Layout::from_index(v).map_or(-1, |l| h(p).prefs.set_layout(l) as c_int) }
#[no_mangle] pub extern "C" fn baura_prefs_get_theme(p: *mut BauraServices) -> u32 { h(p).prefs.theme.index() }
#[no_mangle] pub extern "C" fn baura_prefs_set_theme(p: *mut BauraServices, v: u32) -> c_int { Theme::from_index(v).map_or(-1, |l| h(p).prefs.set_theme(l) as c_int) }
#[no_mangle] pub extern "C" fn baura_prefs_get_icon_theme(p: *mut BauraServices) -> u32 { h(p).prefs.icon_theme.index() }
#[no_mangle] pub extern "C" fn baura_prefs_set_icon_theme(p: *mut BauraServices, v: u32) -> c_int { IconTheme::from_index(v).map_or(-1, |l| h(p).prefs.set_icon_theme(l) as c_int) }
#[no_mangle] pub extern "C" fn baura_prefs_get_density(p: *mut BauraServices) -> u32 { h(p).prefs.density.index() }
#[no_mangle] pub extern "C" fn baura_prefs_set_density(p: *mut BauraServices, v: u32) -> c_int { Density::from_index(v).map_or(-1, |l| h(p).prefs.set_density(l) as c_int) }
#[no_mangle] pub extern "C" fn baura_prefs_is_rtl(p: *mut BauraServices) -> c_int { (h(p).prefs.effective_direction() == Direction::Rtl) as c_int }
#[no_mangle] pub unsafe extern "C" fn baura_prefs_set_language(p: *mut BauraServices, lang: *const c_char) -> c_int { h(p).prefs.set_language(cstr(lang)) as c_int }
#[no_mangle] pub unsafe extern "C" fn baura_prefs_get_language(p: *mut BauraServices, buf: *mut c_char, cap: usize) -> usize { put(&h(p).prefs.language, buf, cap) }
#[no_mangle] pub extern "C" fn baura_prefs_text_scale(p: *mut BauraServices) -> u32 { h(p).prefs.text_scale_pct as u32 }
#[no_mangle] pub extern "C" fn baura_prefs_set_text_scale(p: *mut BauraServices, pct: u32) -> c_int { h(p).prefs.set_text_scale(pct.min(u16::MAX as u32) as u16) as c_int }
#[no_mangle] pub unsafe extern "C" fn baura_prefs_serialize(p: *mut BauraServices, buf: *mut c_char, cap: usize) -> usize { put(&h(p).prefs.serialize(), buf, cap) }
/// Retourne le nombre de lignes invalides (0 = tout bon) ; les préférences sont remplacées (la version continue de croître).
#[no_mangle] pub unsafe extern "C" fn baura_prefs_load(p: *mut BauraServices, text: *const c_char) -> c_int { let s = h(p); let (mut n, issues) = Prefs::parse(cstr(text)); n.version = s.prefs.version + 1; s.prefs = n; issues.len() as c_int }

// ---- moniteur
#[no_mangle] pub extern "C" fn baura_monitor_tick(p: *mut BauraServices, now_ms: u64) { let s = h(p); s.monitor.tick(now_ms); s.groups = s.monitor.groups(); }
#[no_mangle] pub extern "C" fn baura_monitor_snapshot(p: *mut BauraServices, out: *mut BauraSnapshot) {
    let l = &h(p).monitor.last; if out.is_null() { return; }
    unsafe { *out = BauraSnapshot { t_ms: l.t_ms, cpu_total_pct: l.cpu_total_pct, mem_total_kib: l.mem_total_kib, mem_used_kib: l.mem_used_kib, swap_total_kib: l.swap_total_kib, swap_used_kib: l.swap_used_kib, net_rx_bps: l.net_rx_bps, net_tx_bps: l.net_tx_bps, disk_read_bps: l.disk_read_bps, disk_write_bps: l.disk_write_bps,
        battery_pct: l.battery_pct.unwrap_or(-1.0), temp_c: l.temp_c.unwrap_or(-1.0), cores: l.cpu_cores_pct.len() as u32 }; }
}
#[no_mangle] pub extern "C" fn baura_monitor_core(p: *mut BauraServices, i: u32) -> f32 { h(p).monitor.last.cpu_cores_pct.get(i as usize).copied().unwrap_or(0.0) }
/// kind : 0 = CPU %, 1 = mémoire %, 2 = réseau B/s, 3 = disque B/s. Copie jusqu'à `cap` points (les plus anciens d'abord) ; retourne le total disponible.
#[no_mangle] pub unsafe extern "C" fn baura_monitor_history(p: *mut BauraServices, kind: u32, out: *mut f32, cap: usize) -> usize {
    let m = &h(p).monitor; let v = match kind { 0 => m.cpu.to_vec(), 1 => m.mem.to_vec(), 2 => m.net.to_vec(), _ => m.disk.to_vec() };
    if !out.is_null() { for (i, x) in v.iter().take(cap).enumerate() { *out.add(i) = *x; } } v.len()
}
#[no_mangle] pub extern "C" fn baura_monitor_group_count(p: *mut BauraServices) -> usize { h(p).groups.len() }
#[no_mangle] pub unsafe extern "C" fn baura_monitor_group(p: *mut BauraServices, i: usize, out: *mut BauraGroup, name: *mut c_char, name_cap: usize, icon: *mut c_char, icon_cap: usize) -> c_int {
    let Some(g) = h(p).groups.get(i) else { return -1 }; if !out.is_null() { *out = BauraGroup { cpu_pct: g.cpu_pct, rss_kib: g.rss_kib, processes: g.processes as u32, threads: g.threads, impact: g.impact as u32 }; } put(&g.name, name, name_cap); put(baura_icons::by_display_name(&g.name).id, icon, icon_cap); 0
}
/// 0 = terminé · 1 = composant protégé · 2 = introuvable
#[no_mangle] pub unsafe extern "C" fn baura_monitor_terminate(p: *mut BauraServices, name: *const c_char) -> c_int {
    let s = h(p); let r = match s.monitor.terminate_group(cstr(name)) { Ok(_) => 0, Err(TerminateError::Protected) => 1, Err(TerminateError::NotFound) => 2 }; s.groups = s.monitor.groups(); r
}

// ---- barre des tâches / Démarrer / recherche
#[no_mangle] pub unsafe extern "C" fn baura_window_open(p: *mut BauraServices, id: u32, app_id: *const c_char, title: *const c_char) { let s = h(p); for w in &mut s.windows { w.focused = false; } s.windows.push(Window { id, app_id: cstr(app_id).into(), title: cstr(title).into(), minimized: false, focused: true, badge: 0 }); let a = cstr(app_id).to_string(); s.recent.retain(|r| *r != a); s.recent.insert(0, a); }
#[no_mangle] pub extern "C" fn baura_window_close(p: *mut BauraServices, id: u32) { h(p).windows.retain(|w| w.id != id); }
#[no_mangle] pub extern "C" fn baura_window_set_badge(p: *mut BauraServices, id: u32, badge: u32) { if let Some(w) = h(p).windows.iter_mut().find(|w| w.id == id) { w.badge = badge; } }
#[no_mangle] pub extern "C" fn baura_taskbar_refresh(p: *mut BauraServices) -> usize { let s = h(p); s.tb = taskbar(&s.prefs.taskbar_pins, &s.windows); s.tb.len() }
#[no_mangle] pub unsafe extern "C" fn baura_taskbar_item(p: *mut BauraServices, i: usize, out: *mut BauraTaskbarItem, app_id: *mut c_char, id_cap: usize, icon: *mut c_char, icon_cap: usize) -> c_int {
    let Some(t) = h(p).tb.get(i) else { return -1 }; if !out.is_null() { *out = BauraTaskbarItem { pinned: t.pinned as u8, running: t.running as u8, focused: t.focused as u8, attention: t.attention as u8, window_count: t.windows.len() as u32, badge: t.badge }; } put(&t.app_id, app_id, id_cap); put(t.icon, icon, icon_cap); 0
}
/// Action : 0 lancer · 1 focaliser · 2 réduire · 3 restaurer · 4 parcourir les fenêtres. `*window` reçoit la fenêtre concernée.
#[no_mangle] pub unsafe extern "C" fn baura_taskbar_click(p: *mut BauraServices, i: usize, window: *mut u32) -> c_int {
    let s = h(p); let Some(t) = s.tb.get(i) else { return -1 }; let set = |v: u32| if !window.is_null() { *window = v };
    match click(t, &s.windows) { ClickAction::Launch => 0, ClickAction::Focus(w) => { set(w); 1 } ClickAction::Minimize(w) => { set(w); 2 } ClickAction::Restore(w) => { set(w); 3 } ClickAction::CycleWindows(v) => { set(v[0]); 4 } }
}
#[no_mangle] pub unsafe extern "C" fn baura_search(p: *mut BauraServices, query: *const c_char, limit: usize) -> usize { let s = h(p); s.hits = search(&s.items, cstr(query), limit, &s.recent); s.hits.len() }
#[no_mangle] pub unsafe extern "C" fn baura_search_hit(p: *mut BauraServices, i: usize, id: *mut c_char, id_cap: usize, title: *mut c_char, title_cap: usize, icon: *mut c_char, icon_cap: usize) -> c_int {
    let Some(x) = h(p).hits.get(i) else { return -1 }; put(&x.id, id, id_cap); put(&x.title, title, title_cap); put(x.icon, icon, icon_cap); x.kind as c_int
}
#[no_mangle] pub unsafe extern "C" fn baura_icon_resolve(app_id: *const c_char, categories: *const c_char, buf: *mut c_char, cap: usize) -> usize { put(baura_icons::resolve(cstr(app_id), cstr(categories)).0.id, buf, cap) }

// ---- notifications
#[no_mangle] pub unsafe extern "C" fn baura_notify(p: *mut BauraServices, app: *const c_char, title: *const c_char, body: *const c_char, t_ms: u64, urgent: c_int) -> c_int { let (_, show) = h(p).notifs.push(cstr(app), cstr(title), cstr(body), t_ms, urgent != 0); show as c_int }
#[no_mangle] pub extern "C" fn baura_notif_unread(p: *mut BauraServices) -> usize { h(p).notifs.unread() }
#[no_mangle] pub extern "C" fn baura_notif_count(p: *mut BauraServices) -> usize { h(p).notifs.list().len() }
#[no_mangle] pub extern "C" fn baura_notif_set_dnd(p: *mut BauraServices, on: c_int) { h(p).notifs.dnd = on != 0; }
#[no_mangle] pub extern "C" fn baura_notif_mark_read(p: *mut BauraServices) { h(p).notifs.mark_all_read(); }
#[no_mangle] pub extern "C" fn baura_notif_clear(p: *mut BauraServices) { h(p).notifs.clear(); }

// ---- traductions
#[no_mangle] pub unsafe extern "C" fn baura_tr(p: *mut BauraServices, key: *const c_char, buf: *mut c_char, cap: usize) -> usize { let s = h(p); let k = cstr(key); put(s.i18n.tr(&s.prefs.language, k), buf, cap) }
#[no_mangle] pub unsafe extern "C" fn baura_tr_n(p: *mut BauraServices, key: *const c_char, n: u64, buf: *mut c_char, cap: usize) -> usize { let s = h(p); put(&s.i18n.tr_n(&s.prefs.language, cstr(key), n), buf, cap) }
#[no_mangle] pub extern "C" fn baura_catalog_count(p: *mut BauraServices) -> usize { h(p).catalog.len() }

#[cfg(test)]
mod tests {
    use super::*; use std::ffi::CString;
    fn c(s: &str) -> CString { CString::new(s).unwrap() }
    #[test] fn full_flow_through_c_abi() { unsafe {
        let s = baura_new(0, 42); assert_eq!(baura_prefs_get_layout(s), 0); assert_eq!(baura_prefs_set_layout(s, 1), 1); assert_eq!(baura_prefs_set_layout(s, 1), 0); assert_eq!(baura_prefs_set_layout(s, 99), -1);
        for t in 1..=5 { baura_monitor_tick(s, t * 1000); } let mut sn = BauraSnapshot::default(); baura_monitor_snapshot(s, &mut sn); assert_eq!(sn.cores, 8); assert!(sn.mem_used_kib > 0);
        assert_eq!(baura_monitor_history(s, 0, std::ptr::null_mut(), 0), 5); let n = baura_monitor_group_count(s); assert!(n > 5);
        let (mut g, mut name) = (BauraGroup::default(), [0 as c_char; 64]); assert_eq!(baura_monitor_group(s, 0, &mut g, name.as_mut_ptr(), 64, std::ptr::null_mut(), 0), 0); assert_eq!(baura_monitor_terminate(s, c("Système").as_ptr()), 1); assert_eq!(baura_monitor_terminate(s, c("rien").as_ptr()), 2);
        baura_window_open(s, 1, c("houb").as_ptr(), c("Accueil").as_ptr()); assert_eq!(baura_taskbar_refresh(s), 6); let mut it = BauraTaskbarItem::default(); let (mut a, mut ic) = ([0 as c_char; 32], [0 as c_char; 32]); assert_eq!(baura_taskbar_item(s, 1, &mut it, a.as_mut_ptr(), 32, ic.as_mut_ptr(), 32), 0); assert_eq!(it.running, 1);
        let mut w = 0u32; assert_eq!(baura_taskbar_click(s, 1, &mut w), 2); assert_eq!(w, 1); assert_eq!(baura_taskbar_click(s, 0, &mut w), 0);
        assert!(baura_search(s, c("moni").as_ptr(), 5) >= 1); let (mut id, mut ti, mut icn) = ([0 as c_char; 32], [0 as c_char; 64], [0 as c_char; 32]); assert_eq!(baura_search_hit(s, 0, id.as_mut_ptr(), 32, ti.as_mut_ptr(), 64, icn.as_mut_ptr(), 32), 0); assert_eq!(CStr::from_ptr(id.as_ptr()).to_str().unwrap(), "monitor");
        baura_prefs_set_language(s, c("ar").as_ptr()); assert_eq!(baura_prefs_is_rtl(s), 1); let mut b = [0 as c_char; 128]; let need = baura_tr_n(s, c("notif.count").as_ptr(), 2, b.as_mut_ptr(), 128); assert_eq!(CStr::from_ptr(b.as_ptr()).to_str().unwrap().len(), need); assert_eq!(CStr::from_ptr(b.as_ptr()).to_str().unwrap(), "إشعاران");
        let mut buf = vec![0 as c_char; 1024]; let len = baura_prefs_serialize(s, buf.as_mut_ptr(), 1024); assert!(len > 50 && len < 1024); let v0 = baura_prefs_version(s); assert_eq!(baura_prefs_load(s, buf.as_ptr()), 0); assert!(baura_prefs_version(s) > v0); assert_eq!(baura_prefs_get_layout(s), 1);
        assert_eq!(baura_notify(s, c("mail").as_ptr(), c("t").as_ptr(), c("b").as_ptr(), 1, 0), 1); assert_eq!(baura_notif_unread(s), 1);
        let mut small = [0 as c_char; 4]; assert!(baura_prefs_get_language(s, small.as_mut_ptr(), 4) == 2); assert_eq!(CStr::from_ptr(small.as_ptr()).to_str().unwrap(), "ar");
        baura_free(s); } }
}
