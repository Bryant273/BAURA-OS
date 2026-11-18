// BGE – scene.hpp · arbre de widgets, mise en page flex, événements, boucle d'image (Engine).
#pragma once
#include "gfx.hpp"

namespace bge {

// ---- Texte : interface à brancher sur FreeType/HarfBuzz (ou DirectWrite/CoreText) ----------
struct TextProvider {
  virtual ~TextProvider() = default;
  virtual Vec2 measure(const std::string& s, float size, bool bold) = 0;
  virtual void draw(Painter& p, const std::string& s, Vec2 pos, float size, bool bold, Color c) = 0;
};
// Démo sans police : dessine des « barres de texte » proportionnelles (placeholder honnête).
struct PlaceholderText : TextProvider {
  Vec2 measure(const std::string& s, float size, bool) override { return {float(s.size()) * size * .56f, size * 1.25f}; }
  void draw(Painter& p, const std::string& s, Vec2 pos, float size, bool bold, Color c) override {
    float x = pos.x; for (char ch : s) { float w = size * .56f; if (ch != ' ') p.fillRRect({x + 1, pos.y + size * .32f, w - 2, size * (bold ? .62f : .5f)}, 2, c.alpha(.85f)); x += w; }
  }
};

enum class Dir { Row, Col };
enum class Align { Start, Center, End, Stretch };
enum class Justify { Start, Center, End, Between };
struct Insets { float l = 0, t = 0, r = 0, b = 0; Insets(float a = 0) : l(a), t(a), r(a), b(a) {} Insets(float x, float y) : l(x), t(y), r(x), b(y) {} };

struct Style {
  Dir dir = Dir::Row; Align align = Align::Center; Justify justify = Justify::Start;
  float gap = 0, w = -1, h = -1, grow = 0; Insets pad;
  bool absolute = false; float x = 0, y = 0;  // positionnement absolu (équiv. position:fixed/absolute)
};

struct Event {
  enum Type { Move, Down, Up, Wheel, Key } type = Move; Vec2 pos; int button = 0; float dy = 0; int key = 0;
};

struct Ctx { const Theme& th; TextProvider& text; Animator& anim; };

class Widget {
 public:
  Style style; Rect frame; float opacity = 1, scale = 1; bool visible = true, hover = false, pressed = false; Widget* parent = nullptr;
  std::vector<std::unique_ptr<Widget>> kids;
  virtual ~Widget() = default;

  template <class T, class... A> T* add(A&&... a) { auto p = std::make_unique<T>(std::forward<A>(a)...); T* r = p.get(); r->parent = this; kids.push_back(std::move(p)); return r; }

  // 1) mesure : taille voulue (contenu) ------------------------------------------------------
  virtual Vec2 measure(Ctx& c) {
    if (style.w >= 0 && style.h >= 0) return {style.w, style.h};
    float main = 0, cross = 0; int n = 0;
    for (auto& k : kids) { if (!k->visible || k->style.absolute) continue; Vec2 m = k->size(c); ++n;
      if (style.dir == Dir::Row) { main += m.x; cross = std::max(cross, m.y); } else { main += m.y; cross = std::max(cross, m.x); } }
    main += style.gap * std::max(0, n - 1);
    float W = style.dir == Dir::Row ? main : cross, H = style.dir == Dir::Row ? cross : main;
    return {style.w >= 0 ? style.w : W + style.pad.l + style.pad.r, style.h >= 0 ? style.h : H + style.pad.t + style.pad.b};
  }
  Vec2 size(Ctx& c) { Vec2 m = measure(c); return {style.w >= 0 ? style.w : m.x, style.h >= 0 ? style.h : m.y}; }

  // 2) placement : distribue l'espace aux enfants (flex simplifié) ---------------------------
  virtual void layout(Ctx& c, Rect box) {
    frame = box; Rect in{box.x + style.pad.l, box.y + style.pad.t, box.w - style.pad.l - style.pad.r, box.h - style.pad.t - style.pad.b};
    bool row = style.dir == Dir::Row; float mainSz = row ? in.w : in.h, crossSz = row ? in.h : in.w;
    std::vector<Widget*> fl; float used = 0, grow = 0;
    for (auto& k : kids) { if (!k->visible) continue; if (k->style.absolute) { Vec2 s = k->size(c); k->layout(c, {box.x + k->style.x, box.y + k->style.y, s.x, s.y}); continue; } fl.push_back(k.get()); }
    std::vector<Vec2> sz; for (auto* k : fl) { sz.push_back(k->size(c)); used += row ? sz.back().x : sz.back().y; grow += k->style.grow; }
    used += style.gap * std::max<int>(0, int(fl.size()) - 1); float left = std::max(0.f, mainSz - used), pos = 0, extraGap = 0;
    if (grow == 0) { if (style.justify == Justify::Center) pos = left / 2; else if (style.justify == Justify::End) pos = left; else if (style.justify == Justify::Between && fl.size() > 1) extraGap = left / (fl.size() - 1); }
    for (size_t i = 0; i < fl.size(); ++i) {
      Widget* k = fl[i]; float m = (row ? sz[i].x : sz[i].y) + (grow > 0 ? left * k->style.grow / grow : 0), cr = row ? sz[i].y : sz[i].x;
      if (style.align == Align::Stretch && (row ? k->style.h : k->style.w) < 0) cr = crossSz;
      float co = style.align == Align::Center ? (crossSz - cr) / 2 : style.align == Align::End ? crossSz - cr : 0;
      k->layout(c, row ? Rect{in.x + pos, in.y + co, m, cr} : Rect{in.x + co, in.y + pos, cr, m}); pos += m + style.gap + extraGap;
    }
  }

  // 3) dessin : pile de transformations puis peinture propre puis enfants -------------------
  virtual void paintSelf(Painter&, Ctx&) {}
  virtual void paintAfter(Painter&, Ctx&) {}
  void paint(Painter& p, Ctx& c) {
    if (!visible || opacity <= 0) return; p.push();
    if (scale != 1) p.scaleAbout(frame.center(), scale);
    if (opacity < 1) p.opacity(opacity);
    paintSelf(p, c); for (auto& k : kids) k->paint(p, c); paintAfter(p, c); p.pop();
  }

  // 4) événements ---------------------------------------------------------------------------
  virtual bool onEvent(const Event&, Ctx&) { return false; }
  Widget* hit(Vec2 q) {
    if (!visible) return nullptr;
    for (size_t i = kids.size(); i-- > 0;) if (Widget* h = kids[i]->hit(q)) return h;
    return frame.contains(q) && interactive() ? this : nullptr;
  }
  virtual bool interactive() const { return false; }
};

// Engine : possède racine + thème + animateur ; une image = update → layout → paint -----------
class Engine {
 public:
  Engine(int w, int h, TextProvider& t) : cv(w, h), text(&t), th(Theme::light()) { root.style.dir = Dir::Col; root.style.align = Align::Stretch; }
  Widget root; Canvas cv; Animator anim; Theme th; TextProvider* text;
  Canvas& frame(float dt, const std::function<void(Painter&)>& wallpaper) {
    Ctx c{th, *text, anim}; anim.update(dt);
    root.layout(c, {0, 0, float(cv.w), float(cv.h)});
    std::fill(cv.px.begin(), cv.px.end(), 0.f); Painter p(cv); wallpaper(p);   // fond d'abord (le verre le floute)
    root.paint(p, c); return cv;
  }
  void dispatch(const Event& e) {
    Ctx c{th, *text, anim}; Widget* h = root.hit(e.pos);
    if (e.type == Event::Move) { if (h != hovered) { if (hovered) hovered->hover = false; hovered = h; if (h) h->hover = true; } }
    if (e.type == Event::Down) { captured = h; if (h) h->pressed = true; }
    Widget* t = (e.type == Event::Up && captured) ? captured : h;
    for (Widget* w = t; w; w = w->parent) if (w->onEvent(e, c)) break;
    if (e.type == Event::Up) { if (captured) captured->pressed = false; captured = nullptr; }
  }
 private:
  Widget *hovered = nullptr, *captured = nullptr;
};

}  // namespace bge
