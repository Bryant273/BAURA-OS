//! Préférences typées de BAURA OS (bureau, thème, icônes, accessibilité, langue). Format texte versionné `clé=valeur`,
//! lecture tolérante (clés inconnues ignorées, valeurs invalides rapportées et remplacées par le défaut), compteur de version
//! pour que l'interface ne redessine que ce qui a changé.
use std::fmt::Write;

macro_rules! choice {
    ($name:ident { $($v:ident = $s:expr),+ $(,)? }) => {
        #[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
        pub enum $name { #[default] $($v),+ }
        impl $name {
            pub const ALL: &'static [$name] = &[$($name::$v),+];
            pub fn as_str(self) -> &'static str { match self { $($name::$v => $s),+ } }
            pub fn parse(s: &str) -> Option<Self> { match s { $($s => Some($name::$v),)+ _ => None } }
            pub fn index(self) -> u32 { Self::ALL.iter().position(|x| *x == self).unwrap() as u32 }
            pub fn from_index(i: u32) -> Option<Self> { Self::ALL.get(i as usize).copied() }
        }
    };
}
// Le PREMIER choix de chaque énumération est le défaut : le bureau par défaut est la barre coupée en deux.
choice!(Layout { SplitDock = "split-dock", FloatingPill = "floating-pill", FullWidth = "full-width" });
choice!(Theme { Auto = "auto", Light = "light", Dark = "dark", HighContrastLight = "hc-light", HighContrastDark = "hc-dark" });
choice!(IconTheme { Baura = "baura", Original = "original" });
choice!(Density { Comfortable = "comfortable", Compact = "compact" });
choice!(Direction { Auto = "auto", Ltr = "ltr", Rtl = "rtl" });

#[derive(Clone, Debug, PartialEq)]
pub struct Prefs {
    pub layout: Layout, pub theme: Theme, pub accent: u32, pub icon_theme: IconTheme, pub density: Density,
    pub language: String, pub direction: Direction, pub text_scale_pct: u16, pub reduce_motion: bool, pub reduce_transparency: bool,
    pub taskbar_pins: Vec<String>, pub version: u64,
}
impl Default for Prefs {
    fn default() -> Self {
        Prefs { layout: Layout::default(), theme: Theme::default(), accent: 0xFF6D66F2, icon_theme: IconTheme::default(), density: Density::default(),
            language: "fr".into(), direction: Direction::default(), text_scale_pct: 100, reduce_motion: false, reduce_transparency: false,
            taskbar_pins: ["files", "houb", "store", "terminal", "monitor", "settings"].iter().map(|s| s.to_string()).collect(), version: 0 }
    }
}
#[derive(Debug, PartialEq)]
pub struct ParseIssue { pub line: usize, pub message: String }

macro_rules! setter {
    ($fn:ident, $field:ident, $t:ty) => { pub fn $fn(&mut self, v: $t) -> bool { if self.$field == v { false } else { self.$field = v; self.version += 1; true } } };
}
impl Prefs {
    setter!(set_layout, layout, Layout); setter!(set_theme, theme, Theme); setter!(set_accent, accent, u32); setter!(set_icon_theme, icon_theme, IconTheme);
    setter!(set_density, density, Density); setter!(set_direction, direction, Direction); setter!(set_reduce_motion, reduce_motion, bool); setter!(set_reduce_transparency, reduce_transparency, bool);
    pub fn set_language(&mut self, l: &str) -> bool { if self.language == l || l.is_empty() || l.len() > 16 { false } else { self.language = l.to_string(); self.version += 1; true } }
    pub fn set_text_scale(&mut self, pct: u16) -> bool { let p = pct.clamp(75, 200); if self.text_scale_pct == p { false } else { self.text_scale_pct = p; self.version += 1; true } }
    pub fn pin(&mut self, app: &str) -> bool { if self.taskbar_pins.iter().any(|p| p == app) { false } else { self.taskbar_pins.push(app.to_string()); self.version += 1; true } }
    pub fn unpin(&mut self, app: &str) -> bool { let n = self.taskbar_pins.len(); self.taskbar_pins.retain(|p| p != app); let ch = n != self.taskbar_pins.len(); if ch { self.version += 1; } ch }
    pub fn move_pin(&mut self, from: usize, to: usize) -> bool { if from >= self.taskbar_pins.len() || to >= self.taskbar_pins.len() || from == to { return false; } let x = self.taskbar_pins.remove(from); self.taskbar_pins.insert(to, x); self.version += 1; true }
    /// Direction effective : `Auto` suit la langue (arabe, hébreu, persan, ourdou → RTL).
    pub fn effective_direction(&self) -> Direction {
        match self.direction { Direction::Auto => if ["ar", "he", "fa", "ur"].iter().any(|l| self.language == *l || self.language.starts_with(&format!("{l}-"))) { Direction::Rtl } else { Direction::Ltr }, d => d }
    }
    pub fn serialize(&self) -> String {
        let mut s = String::from("# BAURA OS — préférences (format 1)\nformat=1\n");
        let _ = writeln!(s, "layout={}\ntheme={}\naccent=#{:08X}\nicon_theme={}\ndensity={}\nlanguage={}\ndirection={}\ntext_scale={}\nreduce_motion={}\nreduce_transparency={}\npins={}",
            self.layout.as_str(), self.theme.as_str(), self.accent, self.icon_theme.as_str(), self.density.as_str(), self.language, self.direction.as_str(), self.text_scale_pct, self.reduce_motion, self.reduce_transparency, self.taskbar_pins.join(","));
        s
    }
    pub fn parse(text: &str) -> (Prefs, Vec<ParseIssue>) {
        let mut p = Prefs::default(); let mut issues = Vec::new();
        for (n, raw) in text.lines().enumerate() {
            let l = raw.trim(); if l.is_empty() || l.starts_with('#') { continue; }
            let Some((k, v)) = l.split_once('=') else { issues.push(ParseIssue { line: n + 1, message: format!("ligne sans « = » : {l}") }); continue };
            let (k, v) = (k.trim(), v.trim()); let mut bad = |what: &str| issues.push(ParseIssue { line: n + 1, message: format!("valeur invalide pour {k} : {v} ({what})") });
            match k {
                "format" => if v != "1" { bad("format inconnu") },
                "layout" => match Layout::parse(v) { Some(x) => p.layout = x, None => bad("choix") },
                "theme" => match Theme::parse(v) { Some(x) => p.theme = x, None => bad("choix") },
                "icon_theme" => match IconTheme::parse(v) { Some(x) => p.icon_theme = x, None => bad("choix") },
                "density" => match Density::parse(v) { Some(x) => p.density = x, None => bad("choix") },
                "direction" => match Direction::parse(v) { Some(x) => p.direction = x, None => bad("choix") },
                "accent" => match u32::from_str_radix(v.trim_start_matches('#'), 16) { Ok(c) => p.accent = if v.trim_start_matches('#').len() <= 6 { 0xFF000000 | c } else { c }, Err(_) => bad("couleur hexadécimale") },
                "language" => if v.is_empty() || v.len() > 16 { bad("code langue") } else { p.language = v.into() },
                "text_scale" => match v.parse::<u16>() { Ok(x) => p.text_scale_pct = x.clamp(75, 200), Err(_) => bad("entier") },
                "reduce_motion" => match v.parse() { Ok(x) => p.reduce_motion = x, Err(_) => bad("true/false") },
                "reduce_transparency" => match v.parse() { Ok(x) => p.reduce_transparency = x, Err(_) => bad("true/false") },
                "pins" => p.taskbar_pins = v.split(',').filter(|s| !s.is_empty()).map(String::from).collect(),
                _ => {}   // clé inconnue : conservée pour compatibilité ascendante (ignorée)
            }
        }
        (p, issues)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn default_is_split_dock() { assert_eq!(Prefs::default().layout, Layout::SplitDock); }
    #[test] fn roundtrip() {
        let mut p = Prefs::default(); p.set_layout(Layout::FloatingPill); p.set_theme(Theme::Dark); p.set_language("ar"); p.set_text_scale(130); p.pin("calculator");
        let (q, issues) = Prefs::parse(&p.serialize()); assert!(issues.is_empty(), "{issues:?}"); assert_eq!(Prefs { version: 0, ..q }, Prefs { version: 0, ..p });
    }
    #[test] fn version_only_on_change() { let mut p = Prefs::default(); assert!(!p.set_layout(Layout::SplitDock)); assert_eq!(p.version, 0); assert!(p.set_layout(Layout::FullWidth)); assert_eq!(p.version, 1); }
    #[test] fn tolerant_parse() {
        let (p, i) = Prefs::parse("layout=banane\nfutur=1\ntheme=dark\nbidon\ntext_scale=900\n");
        assert_eq!(p.layout, Layout::SplitDock); assert_eq!(p.theme, Theme::Dark); assert_eq!(p.text_scale_pct, 200); assert_eq!(i.len(), 2);
    }
    #[test] fn rtl_follows_language() { let mut p = Prefs::default(); assert_eq!(p.effective_direction(), Direction::Ltr); p.set_language("ar"); assert_eq!(p.effective_direction(), Direction::Rtl); p.set_direction(Direction::Ltr); assert_eq!(p.effective_direction(), Direction::Ltr); }
    #[test] fn pins() { let mut p = Prefs::default(); assert!(p.unpin("store")); assert!(!p.unpin("store")); assert!(p.move_pin(0, 2)); assert_eq!(p.taskbar_pins[2], "files"); }
}
