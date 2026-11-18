// BGE – port Rust (cœur) : jetons, courbes CSS, symbole BAURA.  `rustc --edition 2021 --test bge_core.rs && ./bge_core`
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Color { pub r: f32, pub g: f32, pub b: f32, pub a: f32 }
impl Color {
    pub const fn hex(rgb: u32, a: f32) -> Self { Color { r: ((rgb >> 16) & 255) as f32 / 255.0, g: ((rgb >> 8) & 255) as f32 / 255.0, b: (rgb & 255) as f32 / 255.0, a } }
    pub fn mix(self, o: Color, t: f32) -> Color { Color { r: self.r + (o.r - self.r) * t, g: self.g + (o.g - self.g) * t, b: self.b + (o.b - self.b) * t, a: self.a + (o.a - self.a) * t } }
}
pub struct Brand;
impl Brand {
    pub const INDIGO: Color = Color::hex(0x4F46E5, 1.0);
    pub const VIOLET: Color = Color::hex(0x8B5CF6, 1.0);
    pub const LAVANDE: Color = Color::hex(0xC4B5FD, 1.0);
    pub const OR: Color = Color::hex(0xAB8E18, 1.0);
}
/// cubic-bezier(x1,y1,x2,y2) comme en CSS.
pub struct CubicBezier(pub f32, pub f32, pub f32, pub f32);
impl CubicBezier {
    pub const SPRING: CubicBezier = CubicBezier(0.3, 1.45, 0.45, 1.0);
    pub const EASE_OUT: CubicBezier = CubicBezier(0.16, 1.0, 0.3, 1.0);
    pub fn at(&self, x: f32) -> f32 {
        if x <= 0.0 { return 0.0 } if x >= 1.0 { return 1.0 }
        let b = |a: f32, c: f32, t: f32| { let u = 1.0 - t; 3.0 * u * u * t * a + 3.0 * u * t * t * c + t * t * t };
        let (mut lo, mut hi, mut t) = (0.0, 1.0, x);
        for _ in 0..24 { let v = b(self.0, self.2, t); if (v - x).abs() < 1e-5 { break } if v < x { lo = t } else { hi = t } t = (lo + hi) / 2.0; }
        b(self.1, self.3, t)
    }
}
/// Le symbole en SVG (viewBox 150 29 199 420).
pub fn logo_svg() -> String {
    format!(r##"<svg xmlns="http://www.w3.org/2000/svg" viewBox="150 29 199 420"><rect x="150" y="29" width="48" height="420" rx="24" fill="#4F46E5"/><path d="M248 29A101 102.5 0 0 1 248 234Z" fill="#8B5CF6"/><path d="M248 245A101 102 0 0 1 248 449Z" fill="#C4B5FD"/><rect x="316" y="221" width="33" height="36" rx="15" fill="#AB8E18"/></svg>"##)
}
#[cfg(test)]
mod t { use super::*;
    #[test] fn spring_overshoots() { let m = (0..100).map(|i| CubicBezier::SPRING.at(i as f32 / 100.0)).fold(0.0, f32::max); assert!(m > 1.0 && m < 1.2); }
    #[test] fn endpoints() { assert_eq!(CubicBezier::EASE_OUT.at(0.0), 0.0); assert_eq!(CubicBezier::EASE_OUT.at(1.0), 1.0); } }
