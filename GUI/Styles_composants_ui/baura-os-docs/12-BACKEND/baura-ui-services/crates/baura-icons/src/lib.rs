//! Résolution des identifiants d'application -> icône BAURA. Les tables (`data.rs`) sont GÉNÉRÉES par
//! 11-ICONES-APPLICATIONS/tools/gen_appicons.py : le même dessin sert côté C++ (rendu) et Rust (logique).
mod data;
pub use data::{IconInfo, ALIASES, CATEGORY_FALLBACK, ICONS, KNOWN_APPS};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Match { Exact, Alias, KnownApp, Category, Fallback }

fn norm(id: &str) -> String {
    let mut s = id.trim().to_ascii_lowercase();
    for suf in [".desktop", ".appimage", "-bin", "-stable"] { if let Some(x) = s.strip_suffix(suf) { s = x.to_string(); } }
    s
}
/// Icône d'une application à partir de son NOM AFFICHÉ (utile pour les groupes du moniteur) : nom exact, début de nom, puis résolution par id.
pub fn by_display_name(name: &str) -> &'static IconInfo {
    let n = name.trim();
    if n == "Système" { return by_id("settings").unwrap(); }
    if let Some(i) = ICONS.iter().find(|i| !i.standalone && i.name.eq_ignore_ascii_case(n)) { return i; }
    if let Some(i) = ICONS.iter().find(|i| !i.standalone && i.name.to_lowercase().starts_with(&n.to_lowercase())) { return i; }
    resolve(&n.to_lowercase(), "").0
}
pub fn by_id(id: &str) -> Option<&'static IconInfo> { ICONS.iter().find(|i| i.id == id) }
/// id exact -> alias freedesktop/Papirus -> application connue -> catégories `.desktop` (« A;B ») -> `app-generic`.
pub fn resolve(app_id: &str, desktop_categories: &str) -> (&'static IconInfo, Match) {
    let n = norm(app_id);
    let last = n.rsplit('.').next().unwrap_or(&n).to_string();   // org.gnome.Nautilus -> nautilus
    if let Some(i) = by_id(&n) { return (i, Match::Exact); }
    if let Some((_, to)) = ALIASES.iter().find(|(a, _)| a.to_ascii_lowercase() == n) { if let Some(i) = by_id(to) { return (i, Match::Alias); } }
    for key in [&n, &last] { if let Some((_, to)) = KNOWN_APPS.iter().find(|(a, _)| a == key) { if let Some(i) = by_id(to) { return (i, Match::KnownApp); } } }
    for c in desktop_categories.split(';').filter(|c| !c.is_empty()) {
        if let Some((_, to)) = CATEGORY_FALLBACK.iter().find(|(a, _)| *a == c) { if let Some(i) = by_id(to) { return (i, Match::Category); } }
    }
    (by_id("app-generic").expect("app-generic"), Match::Fallback)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn exact_alias_known() {
        assert_eq!(resolve("files", "").1, Match::Exact);
        assert_eq!(resolve("org.gnome.Nautilus", "").0.id, "files");
        assert_eq!(resolve("firefox", "").0.id, "browser");
        assert_eq!(resolve("VLC.desktop", "").0.id, "video-player");
    }
    #[test] fn category_then_fallback() {
        let (i, m) = resolve("monappli-inconnue", "Utility;Game;"); assert_eq!((i.id, m), ("app-generic", Match::Category));
        assert_eq!(resolve("monappli-inconnue", "Game;").0.id, "game");
        assert_eq!(resolve("zzz", "").1, Match::Fallback);
    }
    #[test] fn every_mapping_target_exists() {
        for (_, to) in KNOWN_APPS.iter().chain(ALIASES.iter()).chain(CATEGORY_FALLBACK.iter()) { assert!(by_id(to).is_some(), "cible inconnue {to}"); }
    }
    #[test] fn display_names() { assert_eq!(by_display_name("HOUB (navigateur)").id, "houb"); assert_eq!(by_display_name("Éditeur de code").id, "code-editor"); assert_eq!(by_display_name("Fichiers").id, "files"); assert_eq!(by_display_name("Système").id, "settings"); assert_eq!(by_display_name("BMedia").id, "media"); assert_eq!(by_display_name("inconnu-xyz").id, "app-generic"); }
    #[test] fn ids_unique() { let mut v: Vec<_> = ICONS.iter().map(|i| i.id).collect(); v.sort(); let n = v.len(); v.dedup(); assert_eq!(n, v.len()); }
}
