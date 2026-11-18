//! Modèle du shell : barre des tâches (épinglées + fenêtres ouvertes), menu Démarrer, recherche classée, centre de notifications.
//! Aucune dépendance graphique : l'interface C++ lit ces vues et dessine (principe « snapshot + commandes »).
use baura_icons::{resolve, ICONS};

#[derive(Clone, Debug, PartialEq)]
pub struct App { pub id: String, pub name: String, pub category: String, pub icon: &'static str }
/// Catalogue par défaut : toutes les icônes AVEC tuile (= vraies applications), hors dossiers et types de fichiers.
pub fn default_catalog() -> Vec<App> {
    ICONS.iter().filter(|i| !i.standalone).map(|i| App { id: i.id.to_string(), name: i.name.to_string(), category: i.category.to_string(), icon: i.id }).collect()
}
#[derive(Clone, Debug, PartialEq)]
pub struct Window { pub id: u32, pub app_id: String, pub title: String, pub minimized: bool, pub focused: bool, pub badge: u32 }

// ------------------------------------------------------------------ barre des tâches
#[derive(Clone, Debug, PartialEq)]
pub struct TaskbarItem { pub app_id: String, pub icon: &'static str, pub pinned: bool, pub windows: Vec<u32>, pub running: bool, pub focused: bool, pub badge: u32, pub attention: bool }
pub fn taskbar(pins: &[String], windows: &[Window]) -> Vec<TaskbarItem> {
    let mut items: Vec<TaskbarItem> = pins.iter().map(|p| TaskbarItem { app_id: p.clone(), icon: resolve(p, "").0.id, pinned: true, windows: vec![], running: false, focused: false, badge: 0, attention: false }).collect();
    for w in windows {
        let idx = match items.iter().position(|i| i.app_id == w.app_id) { Some(i) => i, None => { items.push(TaskbarItem { app_id: w.app_id.clone(), icon: resolve(&w.app_id, "").0.id, pinned: false, windows: vec![], running: false, focused: false, badge: 0, attention: false }); items.len() - 1 } };
        let it = &mut items[idx]; it.windows.push(w.id); it.running = true; it.focused |= w.focused; it.badge += w.badge; it.attention |= w.badge > 0 && !w.focused;
    }
    items
}
/// Clic sur une entrée : lancer / restaurer / focaliser / réduire — la décision est dans le modèle pour être testée sans interface.
#[derive(Debug, PartialEq, Eq)] pub enum ClickAction { Launch, Focus(u32), Minimize(u32), Restore(u32), CycleWindows(Vec<u32>) }
pub fn click(item: &TaskbarItem, windows: &[Window]) -> ClickAction {
    match item.windows.len() { 0 => ClickAction::Launch,
        1 => { let w = windows.iter().find(|w| w.id == item.windows[0]).unwrap(); if w.minimized { ClickAction::Restore(w.id) } else if w.focused { ClickAction::Minimize(w.id) } else { ClickAction::Focus(w.id) } }
        _ => ClickAction::CycleWindows(item.windows.clone()) }
}

// ------------------------------------------------------------------ recherche
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)] pub enum Kind { App = 0, Setting = 1, File = 2 }
#[derive(Clone, Debug, PartialEq)] pub struct Hit { pub kind: Kind, pub id: String, pub title: String, pub score: u32, pub icon: &'static str }
fn fold(s: &str) -> String { s.chars().map(|c| match c { 'é' | 'è' | 'ê' | 'ë' | 'É' | 'È' => 'e', 'à' | 'â' | 'ä' | 'À' => 'a', 'î' | 'ï' => 'i', 'ô' | 'ö' => 'o', 'ù' | 'û' | 'ü' => 'u', 'ç' => 'c', c => c.to_ascii_lowercase() }).collect() }
/// Score : préfixe (1000) > début de mot (700) > sous-chaîne (400) > sous-séquence (150) ; 0 = pas de résultat. Insensible à la casse et aux accents.
pub fn score(query: &str, text: &str) -> u32 {
    let (q, t) = (fold(query.trim()), fold(text)); if q.is_empty() { return 0; }
    let len_bonus = 100u32.saturating_sub(t.len() as u32);
    if t.starts_with(&q) { return 1000 + len_bonus; }
    if t.split(|c: char| !c.is_alphanumeric()).any(|w| w.starts_with(&q)) { return 700 + len_bonus; }
    if t.contains(&q) { return 400 + len_bonus; }
    let mut it = t.chars(); if q.chars().all(|c| it.any(|x| x == c)) { 150 } else { 0 }
}
pub struct Item { pub kind: Kind, pub id: String, pub title: String, pub keywords: String, pub icon: &'static str }
pub fn search(items: &[Item], query: &str, limit: usize, recent: &[String]) -> Vec<Hit> {
    let mut v: Vec<Hit> = items.iter().filter_map(|i| { let s = score(query, &i.title).max(score(query, &i.keywords) / 2); if s == 0 { return None; }
        let boost = if recent.iter().any(|r| *r == i.id) { 120 } else { 0 }; Some(Hit { kind: i.kind, id: i.id.clone(), title: i.title.clone(), score: s + boost, icon: i.icon }) }).collect();
    v.sort_by(|a, b| b.score.cmp(&a.score).then(a.kind.cmp(&b.kind)).then(a.title.cmp(&b.title))); v.truncate(limit); v
}
pub fn items_from_catalog(apps: &[App]) -> Vec<Item> { apps.iter().map(|a| Item { kind: Kind::App, id: a.id.clone(), title: a.name.clone(), keywords: format!("{} {}", a.category, a.id), icon: a.icon }).collect() }

// ------------------------------------------------------------------ menu Démarrer
#[derive(Clone, Debug, PartialEq)] pub struct StartView { pub pinned: Vec<App>, pub recent: Vec<App>, pub all_by_category: Vec<(String, Vec<App>)> }
pub fn start_view(catalog: &[App], pins: &[String], recent_ids: &[String]) -> StartView {
    let get = |id: &String| catalog.iter().find(|a| a.id == *id).cloned();
    let mut cats: Vec<(String, Vec<App>)> = vec![];
    for a in catalog { match cats.iter_mut().find(|(c, _)| *c == a.category) { Some((_, v)) => v.push(a.clone()), None => cats.push((a.category.clone(), vec![a.clone()])) } }
    cats.sort_by(|a, b| fold(&a.0).cmp(&fold(&b.0))); for (_, v) in cats.iter_mut() { v.sort_by(|a, b| fold(&a.name).cmp(&fold(&b.name))); }
    StartView { pinned: pins.iter().filter_map(get).collect(), recent: recent_ids.iter().filter_map(get).take(6).collect(), all_by_category: cats }
}

// ------------------------------------------------------------------ notifications
#[derive(Clone, Debug, PartialEq)] pub struct Notification { pub id: u64, pub app_id: String, pub title: String, pub body: String, pub t_ms: u64, pub read: bool, pub urgent: bool }
#[derive(Default)] pub struct Notifications { items: Vec<Notification>, next: u64, pub dnd: bool }
impl Notifications {
    pub const CAP: usize = 50;
    /// Ajoute une notification ; renvoie `Some(id)` si une bulle doit s'afficher (pas en « ne pas déranger », sauf urgence).
    pub fn push(&mut self, app: &str, title: &str, body: &str, t_ms: u64, urgent: bool) -> (u64, bool) {
        self.next += 1; self.items.insert(0, Notification { id: self.next, app_id: app.into(), title: title.into(), body: body.into(), t_ms, read: false, urgent }); self.items.truncate(Self::CAP);
        (self.next, !self.dnd || urgent)
    }
    pub fn unread(&self) -> usize { self.items.iter().filter(|n| !n.read).count() }
    pub fn mark_all_read(&mut self) { for n in &mut self.items { n.read = true; } }
    pub fn dismiss(&mut self, id: u64) -> bool { let n = self.items.len(); self.items.retain(|x| x.id != id); n != self.items.len() }
    pub fn clear(&mut self) { self.items.clear(); }
    pub fn list(&self) -> &[Notification] { &self.items }
    /// Regroupement par application, le plus récent en premier.
    pub fn grouped(&self) -> Vec<(String, Vec<&Notification>)> { let mut g: Vec<(String, Vec<&Notification>)> = vec![]; for n in &self.items { match g.iter_mut().find(|(a, _)| *a == n.app_id) { Some((_, v)) => v.push(n), None => g.push((n.app_id.clone(), vec![n])) } } g }
}
#[cfg(test)]
mod tests {
    use super::*;
    fn w(id: u32, app: &str, min: bool, foc: bool, badge: u32) -> Window { Window { id, app_id: app.into(), title: app.into(), minimized: min, focused: foc, badge } }
    #[test] fn taskbar_merges_pins_and_running() {
        let pins = vec!["files".to_string(), "houb".to_string()]; let t = taskbar(&pins, &[w(1, "houb", false, true, 0), w(2, "houb", false, false, 3), w(3, "calculator", false, false, 0)]);
        assert_eq!(t.len(), 3); assert!(t[1].running && t[1].windows == vec![1, 2] && t[1].focused && t[1].badge == 3); assert!(!t[0].running); assert!(!t[2].pinned && t[2].running);
    }
    #[test] fn click_logic() {
        let ws = [w(1, "files", true, false, 0), w(2, "houb", false, true, 0), w(3, "terminal", false, false, 0), w(4, "terminal", false, false, 0)];
        let t = taskbar(&["notes".to_string(), "files".to_string(), "houb".to_string(), "terminal".to_string()], &ws);
        assert_eq!(click(&t[0], &ws), ClickAction::Launch); assert_eq!(click(&t[1], &ws), ClickAction::Restore(1)); assert_eq!(click(&t[2], &ws), ClickAction::Minimize(2)); assert_eq!(click(&t[3], &ws), ClickAction::CycleWindows(vec![3, 4]));
    }
    #[test] fn score_order() { assert!(score("par", "Paramètres") > score("par", "Aperçu des parts")); assert!(score("moniteur", "Moniteur système") >= 1000); assert!(score("sys", "Moniteur système") >= 700); assert_eq!(score("zzz", "Paramètres"), 0); assert!(score("PARAM", "paramètres") >= 1000); assert!(score("pmt", "Paramètres") == 150); }
    #[test] fn search_ranks_apps() { let cat = default_catalog(); let items = items_from_catalog(&cat); let h = search(&items, "moni", 5, &[]); assert_eq!(h[0].id, "monitor"); let h2 = search(&items, "term", 3, &[]); assert_eq!(h2[0].id, "terminal"); }
    #[test] fn recent_boost() { let items = vec![Item { kind: Kind::App, id: "a".into(), title: "Notes A".into(), keywords: "".into(), icon: "notes" }, Item { kind: Kind::App, id: "b".into(), title: "Notes B".into(), keywords: "".into(), icon: "notes" }]; assert_eq!(search(&items, "notes", 2, &["b".to_string()])[0].id, "b"); }
    #[test] fn catalog_excludes_files() { let c = default_catalog(); assert!(c.len() > 60 && c.iter().all(|a| !a.id.starts_with("file-") && !a.id.starts_with("folder"))); assert!(c.iter().any(|a| a.id == "monitor")); }
    #[test] fn start_view_groups() { let c = default_catalog(); let v = start_view(&c, &["files".into(), "inconnu".into()], &["monitor".into()]); assert_eq!(v.pinned.len(), 1); assert_eq!(v.recent[0].id, "monitor"); assert!(v.all_by_category.windows(2).all(|p| fold(&p[0].0) <= fold(&p[1].0))); }
    #[test] fn notifications_flow() {
        let mut n = Notifications::default(); let (a, show) = n.push("mail", "Nouveau message", "…", 1, false); assert!(show); n.dnd = true; let (_, show2) = n.push("mail", "Autre", "…", 2, false); assert!(!show2); let (_, show3) = n.push("update", "Urgent", "…", 3, true); assert!(show3);
        assert_eq!(n.unread(), 3); assert_eq!(n.grouped().len(), 2); assert!(n.dismiss(a)); n.mark_all_read(); assert_eq!(n.unread(), 0); for i in 0..80 { n.push("x", "t", "b", i, false); } assert_eq!(n.list().len(), Notifications::CAP);
    }
}
