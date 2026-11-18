// BGE – components.hpp · composants BAURA assemblés à partir des primitives (miroir des classes CSS).
#pragma once
#include "scene.hpp"

namespace bge {

inline Paint tile(uint32_t a, uint32_t b) { return Paint::linear(145, Color::hex(a), Color::hex(b)); }

// Boîte générique : fond, rayon, bordure, ombre, verre
struct Box : Widget {
  Paint bg{Color{0, 0, 0, 0}}; float radius = 0; bool glass = false, useGlassStrong = false, shadow = false; Color border{0, 0, 0, 0}; float borderW = 1;
  void paintSelf(Painter& p, Ctx& c) override {
    if (shadow) { p.shadow(frame, radius, c.th.shadowY1, c.th.shadowB1, c.th.shadow1); p.shadow(frame, radius, c.th.shadowY2, c.th.shadowB2, c.th.shadow2); }
    if (glass) p.glass(frame, radius, c.th, useGlassStrong ? c.th.glassStrong : c.th.glass, c.th.glassLine);
    else { p.fillRRect(frame, radius, bg); if (border.a > 0) p.strokeRRect(frame, radius, borderW, border); }
  }
};

struct Label : Widget {
  std::string text; float size = 14; bool bold = false, muted = false;
  Label(std::string t = "", float s = 14, bool b = false, bool m = false) : text(std::move(t)), size(s), bold(b), muted(m) {}
  Vec2 measure(Ctx& c) override { return c.text.measure(text, size, bold); }
  void paintSelf(Painter& p, Ctx& c) override { c.text.draw(p, text, {frame.x, frame.y}, size, bold, muted ? c.th.mut : c.th.fg); }
};

struct IconW : Widget {
  std::string name; float px; int tone = 0;  // 0 = fg, 1 = accent, 2 = blanc
  IconW(std::string n, float s = 18, int t = 0) : name(std::move(n)), px(s), tone(t) { style.w = style.h = s; }
  void paintSelf(Painter& p, Ctx& c) override { p.icon(name.c_str(), frame, tone == 2 ? Color{1, 1, 1, 1} : tone == 1 ? c.th.accent : c.th.fg); }
};

// Bouton à fond au survol / à l'appui (--hover / --press), animé 150 ms
struct Button : Widget {
  float radius = 12, k = 0; std::function<void()> onClick; Color fill{0, 0, 0, 0}; bool wasHover = false;
  bool interactive() const override { return true; }
  void paintSelf(Painter& p, Ctx& c) override {
    if (hover != wasHover) { wasHover = hover; Tween t; t.from = k; t.to = hover ? 1 : 0; t.dur = .15f; t.apply = [this](float v) { k = v; }; c.anim.add(t); }
    if (fill.a > 0) p.fillRRect(frame, radius, fill);
    if (k > 0 || pressed) p.fillRRect(frame, radius, (pressed ? c.th.press : c.th.hover).alpha(pressed ? 1.f : k));
  }
  bool onEvent(const Event& e, Ctx&) override { if (e.type == Event::Up && frame.contains(e.pos) && onClick) { onClick(); return true; } return e.type == Event::Down; }
};

// Pastille d'application (tuile dégradée + icône blanche)
struct AppTile : Widget {
  Paint paint; std::string icon; float side, rad;
  AppTile(Paint p, std::string i, float s = 42, float r = 14) : paint(p), icon(std::move(i)), side(s), rad(r) { style.w = style.h = s; }
  void paintSelf(Painter& p, Ctx& c) override {
    p.shadow(frame, rad, 4, 10, Color{.2f, .15f, .5f, .28f}); p.fillRRect(frame, rad, paint);
    p.strokeRRect(frame, rad, 1, Color{1, 1, 1, .35f}); p.icon(icon.c_str(), frame.inset(side * .24f), Color{1, 1, 1, 1}); (void)c; }
};

// Barre des tâches : pilule verre 52 px, rayon 20, bouton démarrer = symbole BAURA
struct TaskbarPill : Box {
  TaskbarPill() { glass = true; radius = 20; shadow = true; style.h = 52; style.pad = Insets(5, 5); style.gap = 4; }
  void paintAfter(Painter&, Ctx&) override {}
};
struct StartButton : Button {
  StartButton() { style.w = 44; style.h = 42; radius = 14; }
  void paintSelf(Painter& p, Ctx& c) override { Button::paintSelf(p, c); p.logo({frame.x + 13, frame.y + 8, 18, 26}, c.th.b1, c.th.b2, c.th.b3, c.th.b4); }
};
// Application épinglée : tuile + indicateur « en cours » (barre dorée 14×3)
struct DockApp : Widget {
  AppTile* t; bool running = false;
  DockApp(Paint p, const char* icon, bool run) : running(run) { style.w = 42; style.h = 42; t = add<AppTile>(p, icon, 38, 13); }
  void layout(Ctx& c, Rect b) override { Widget::layout(c, b); t->layout(c, {b.x + 2, b.y + 1, 38, 38}); }
  void paintAfter(Painter& p, Ctx& c) override { if (running) p.fillRRect({frame.x + 14, frame.y + 39, 14, 3}, 1.5f, c.th.b4); }
};

// Fenêtre : ombre portée + verre (--win) + barre de titre + 3 boutons + zone de contenu
struct WindowW : Widget {
  std::string title; const char* icon; Paint iconTile; Widget* content = nullptr; float bar = 46, radius = 22;
  WindowW(std::string t, const char* ic, Paint tp) : title(std::move(t)), icon(ic), iconTile(tp) { style.dir = Dir::Col; style.align = Align::Stretch; style.pad = Insets(0, 0); }
  void layout(Ctx& c, Rect b) override { frame = b; if (content) content->layout(c, {b.x, b.y + bar, b.w, b.h - bar}); }
  void paintSelf(Painter& p, Ctx& c) override {
    const Theme& t = c.th; p.shadow(frame, radius, 30, 70, Color{.06f, .04f, .18f, .35f}); p.shadow(frame, radius, t.shadowY1, t.shadowB1, t.shadow1);
    p.glass(frame, radius, t, t.win, t.winLine);
    p.push(); p.clip({frame.x, frame.y, frame.w, bar}); p.fillRRect({frame.x, frame.y, frame.w, bar + radius}, radius, t.winBar); p.pop();
    p.fillRRect({frame.x, frame.y + bar - 1, frame.w, 1}, 0, t.hair);
    Rect ic{frame.x + 14, frame.y + 12, 22, 22}; p.fillRRect(ic, 7, iconTile); p.icon(icon, ic.inset(4), Color{1, 1, 1, 1});
    c.text.draw(p, title, {frame.x + 46, frame.y + 13}, 14, true, t.fg);
    float bx = frame.x + frame.w - 14;  // fermer, agrandir, réduire (droite → gauche)
    struct B { const char* ic; Color bg; } bs[3] = {{"x", t.b4.alpha(.22f)}, {"circle", t.b3.alpha(.35f)}, {"minus", t.b3.alpha(.35f)}};
    for (int i = 0; i < 3; ++i) { Rect r{bx - 32 - i * 38.f, frame.y + 11, 32, 24}; p.fillRRect(r, 9, bs[i].bg); p.icon(bs[i].ic, Rect{r.x + 9, r.y + 5, 14, 14}, t.fg, 2.2f); }
  }
};

// Notification (toast) : 332 px, rayon 20, verre fort, tuile d'app 40 px + titre + texte
struct Toast : Box {
  std::string title, text; AppTile* tile;
  Toast(std::string t, std::string x, Paint p, const char* icon) : title(std::move(t)), text(std::move(x)) {
    glass = true; useGlassStrong = true; radius = 20; shadow = true; style.w = 332; style.h = 66; style.pad = Insets(14, 13); style.gap = 11; style.align = Align::Start;
    tile = add<AppTile>(p, icon, 40, 13);
  }
  void paintAfter(Painter& p, Ctx& c) override { c.text.draw(p, title, {frame.x + 65, frame.y + 13}, 14, true, c.th.fg); c.text.draw(p, text, {frame.x + 65, frame.y + 34}, 12.5f, false, c.th.mut); }
};

// Pilule d'état (haut-droite) : boutons 34×32 rayon 12 + pastille de notification
struct TrayPill : Box {
  TrayPill(std::initializer_list<const char*> icons, int badge) : badge_(badge) {
    glass = true; radius = 16; shadow = true; style.h = 40; style.pad = Insets(4, 4); style.gap = 2;
    for (auto n : icons) { auto* b = add<Button>(); b->style.w = 34; b->style.h = 32; b->radius = 12; b->add<IconW>(n, 17); b->style.align = Align::Center; b->style.justify = Justify::Center; }
  }
  void paintAfter(Painter& p, Ctx& c) override {
    if (!badge_) return; Rect l = kids.back()->frame; Rect b{l.x + l.w - 13, l.y - 5, 16, 16}; p.fillRRect(b, 8, c.th.err);
    c.text.draw(p, std::to_string(badge_), {b.x + 4.5f, b.y + 2.5f}, 9, true, Color{1, 1, 1, 1});
  }
  int badge_;
};

}  // namespace bge
