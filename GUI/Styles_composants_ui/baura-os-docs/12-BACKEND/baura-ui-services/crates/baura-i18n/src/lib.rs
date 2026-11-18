//! Traductions : catalogues par langue, repli lang → en → clé, règles de pluriel (fr, en, ar…) et direction d'écriture.
use std::collections::HashMap;
#[derive(Clone, Copy, Debug, PartialEq, Eq)] pub enum Plural { Zero, One, Two, Few, Many, Other }
pub fn plural(lang: &str, n: u64) -> Plural {
    match lang.split('-').next().unwrap_or(lang) {
        "fr" | "pt" => if n < 2 { Plural::One } else { Plural::Other },
        "ar" => match n { 0 => Plural::Zero, 1 => Plural::One, 2 => Plural::Two, _ => match n % 100 { 3..=10 => Plural::Few, 11..=99 => Plural::Many, _ => Plural::Other } },
        "ja" | "zh" | "ko" => Plural::Other,
        _ => if n == 1 { Plural::One } else { Plural::Other },
    }
}
pub fn is_rtl(lang: &str) -> bool { matches!(lang.split('-').next().unwrap_or(lang), "ar" | "he" | "fa" | "ur") }
fn suffix(p: Plural) -> &'static str { match p { Plural::Zero => "zero", Plural::One => "one", Plural::Two => "two", Plural::Few => "few", Plural::Many => "many", Plural::Other => "other" } }
#[derive(Default)] pub struct Catalog { by_lang: HashMap<String, HashMap<String, String>> }
impl Catalog {
    pub fn add(&mut self, lang: &str, key: &str, text: &str) { self.by_lang.entry(lang.into()).or_default().insert(key.into(), text.into()); }
    /// Charge `clé=texte` (une par ligne, `#` = commentaire, `\n` échappé).
    pub fn load(&mut self, lang: &str, text: &str) -> usize { let mut n = 0; for l in text.lines() { let l = l.trim(); if l.is_empty() || l.starts_with('#') { continue; } if let Some((k, v)) = l.split_once('=') { self.add(lang, k.trim(), &v.trim().replace("\\n", "\n")); n += 1; } } n }
    fn find(&self, lang: &str, key: &str) -> Option<&str> {
        let base = lang.split('-').next().unwrap_or(lang);
        for l in [lang, base, "en"] { if let Some(s) = self.by_lang.get(l).and_then(|m| m.get(key)) { return Some(s); } } None
    }
    pub fn tr<'a>(&'a self, lang: &str, key: &'a str) -> &'a str { self.find(lang, key).unwrap_or(key) }
    /// Pluriel : cherche `clé.<catégorie>` puis `clé.other` ; remplace `{n}`.
    pub fn tr_n(&self, lang: &str, key: &str, n: u64) -> String {
        let k = format!("{key}.{}", suffix(plural(lang, n))); let s = self.find(lang, &k).or_else(|| self.find(lang, &format!("{key}.other"))).unwrap_or(key); s.replace("{n}", &n.to_string())
    }
    pub fn languages(&self) -> Vec<&str> { let mut v: Vec<&str> = self.by_lang.keys().map(|s| s.as_str()).collect(); v.sort(); v }
    pub fn missing(&self, lang: &str) -> Vec<String> { let mut v: Vec<String> = self.by_lang.get("en").map(|m| m.keys().filter(|k| !self.by_lang.get(lang).map_or(false, |l| l.contains_key(*k))).cloned().collect()).unwrap_or_default(); v.sort(); v }
}
pub const FR: &str = include_str!("../catalogs/fr.txt"); pub const EN: &str = include_str!("../catalogs/en.txt"); pub const AR: &str = include_str!("../catalogs/ar.txt");
pub fn builtin() -> Catalog { let mut c = Catalog::default(); c.load("fr", FR); c.load("en", EN); c.load("ar", AR); c }
#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn fallback_chain() { let c = builtin(); assert_eq!(c.tr("fr", "start.title"), "Démarrer"); assert_eq!(c.tr("fr-CA", "start.title"), "Démarrer"); assert_eq!(c.tr("de", "start.title"), "Start"); assert_eq!(c.tr("fr", "cle.inexistante"), "cle.inexistante"); }
    #[test] fn plurals() { assert_eq!(plural("fr", 0), Plural::One); assert_eq!(plural("fr", 2), Plural::Other); assert_eq!(plural("en", 0), Plural::Other); assert_eq!(plural("ar", 0), Plural::Zero); assert_eq!(plural("ar", 2), Plural::Two); assert_eq!(plural("ar", 7), Plural::Few); assert_eq!(plural("ar", 11), Plural::Many); assert_eq!(plural("ar", 100), Plural::Other); assert_eq!(plural("ja", 1), Plural::Other); }
    #[test] fn tr_n() { let c = builtin(); assert_eq!(c.tr_n("fr", "notif.count", 1), "1 notification"); assert_eq!(c.tr_n("fr", "notif.count", 5), "5 notifications"); assert_eq!(c.tr_n("en", "notif.count", 1), "1 notification"); assert_eq!(c.tr_n("ar", "notif.count", 2), "إشعاران"); assert_eq!(c.tr_n("ar", "notif.count", 5), "5 إشعارات"); }
    #[test] fn rtl() { assert!(is_rtl("ar") && is_rtl("fa-IR") && !is_rtl("fr")); }
    #[test] fn catalogs_complete() { let c = builtin(); assert!(c.missing("fr").is_empty(), "{:?}", c.missing("fr")); assert!(c.missing("ar").is_empty(), "{:?}", c.missing("ar")); }
}
