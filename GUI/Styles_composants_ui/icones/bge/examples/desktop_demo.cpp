// Démo BGE : assemble le bureau BAURA (fond + icônes + fenêtre verre + barre des tâches + tray + notification)
// puis écrit desktop_light.ppm / desktop_dark.ppm.   Build : voir CMakeLists.txt
#include <cstdio>
#include "bge/components.hpp"
using namespace bge;

// Fond d'écran : dégradé + symbole géant (barre + demi-disques) + halo doré
static void wallpaper(Painter& p, const Theme& t) {
  Rect all{0, 0, float(p.canvas().w), float(p.canvas().h)};
  p.fillRRect(all, 0, t.dark ? Paint::linear(150, Color::hex(0x1a1450), Color::hex(0x3b1f8f)) : Paint::linear(150, Color::hex(0xF1E4FF), Color::hex(0xA99BFF)));
  p.fillRRect({70, 520, 380, 380}, 190, Color::hex(0xE0BE3A, t.dark ? .10f : .20f));          // halo doré
  Path top = Path::fromSVG("M248 29A101 102.5 0 0 1 248 234Z", 96), bot = Path::fromSVG("M248 245A101 102 0 0 1 248 449Z", 96);
  p.fillPath(top, {1010 - 248 * 3.f, -40 - 29 * 3.f}, 3.f, t.b2.alpha(.42f)); p.fillPath(bot, {1010 - 248 * 3.f, -40 - 29 * 3.f}, 3.f, t.b3.alpha(t.dark ? .22f : .55f));
  p.fillRRect({820, 420, 150, 560}, 75, t.b1.alpha(.55f));                                      // barre
  p.fillRRect({1280, 410, 76, 80}, 34, t.b4.alpha(.85f));                                      // point doré
}

static void build(Engine& e, bool windowOpen) {
  Widget& r = e.root;
  // icônes de bureau
  const char* ico[3] = {"home", "trash", "copy"}; Paint tp[3] = {tile(0x6D66F2, 0x4F46E5), tile(0x9aa0c8, 0x6b7199), tile(0xE0BE3A, 0xAB8E18)};
  for (int i = 0; i < 3; ++i) { auto* a = r.add<AppTile>(tp[i], ico[i], 56, 18); a->style.absolute = true; a->style.x = 28; a->style.y = 20 + i * 104; }
  // fenêtre
  auto* w = r.add<WindowW>("Skrib", "pen", tile(0x60A5FA, 0x2563EB)); w->style.absolute = true; w->style.x = 250; w->style.y = 70; w->style.w = 820; w->style.h = 520;
  auto* body = w->add<Widget>(); body->style.dir = Dir::Col; body->style.align = Align::Stretch; body->style.pad = Insets(18, 12); body->style.gap = 14; w->content = body;
  auto* tb = body->add<Widget>(); tb->style.h = 40; tb->style.gap = 2;
  for (const char* n : {"undo", "redo", "bold", "italic", "underline", "alignL", "alignC", "alignR", "listUl", "table", "image", "link"}) { auto* b = tb->add<Button>(); b->style.w = 36; b->style.h = 34; b->radius = 11; b->style.justify = Justify::Center; b->add<IconW>(n, 18); }
  auto* page = body->add<Box>(); page->bg = Color{1, 1, 1, e.th.dark ? .06f : .75f}; page->radius = 16; page->border = e.th.hair; page->style.grow = 1; page->style.pad = Insets(34, 28); page->style.dir = Dir::Col; page->style.align = Align::Start; page->style.gap = 14;
  page->add<Label>("Bienvenue sur BAURA OS", 26, true); page->add<Label>("Un système simple comme Windows, élégant comme macOS.", 14, false, true);
  page->add<Label>("Le moteur BGE assemble ce document : arbre de widgets, flex, verre, icones vectorielles.", 14);
  (void)windowOpen;
  // pilule d'état + notification
  auto* tray = r.add<TrayPill>(std::initializer_list<const char*>{"volume", "chart", "zap", "mail"}, 4); tray->style.absolute = true; tray->style.x = 1440 - 10 - 152; tray->style.y = 10;
  auto* toast = r.add<Toast>("Bienvenue sur BAURA OS", "Ton bureau est prêt.", tile(0xFB7185, 0xE11D48), "play"); toast->style.absolute = true; toast->style.x = 1098; toast->style.y = 58;
  // barre des tâches
  auto* dock = r.add<TaskbarPill>(); dock->style.absolute = true; dock->style.x = 10; dock->style.y = 900 - 10 - 52;
  dock->add<StartButton>(); auto* sep = dock->add<Box>(); sep->style.w = 1; sep->style.h = 24; sep->bg = e.th.hair;
  dock->add<DockApp>(tile(0x34D399, 0x047857), "table2", false); dock->add<DockApp>(tile(0x60A5FA, 0x2563EB), "pen", true); dock->add<DockApp>(tile(0xA78BFA, 0x6D28D9), "hdd", false); dock->add<DockApp>(tile(0xFB923C, 0xEA580C), "image", false);
}

static void render(bool dark, const char* file) {
  PlaceholderText txt; Engine e(1440, 900, txt); e.th = dark ? Theme::darkTheme() : Theme::light(); build(e, true);
  // animation d'ouverture de la fenêtre (échelle .94 → 1 en 300 ms, courbe --spring) + survol d'une app du dock
  Widget* win = e.root.kids[3].get(); win->scale = .94f; win->opacity = 0;
  Tween t; t.dur = .3f; t.ease = SPRING; t.apply = [win](float v) { win->scale = .94f + .06f * v; win->opacity = std::min(1.f, v * 3); }; e.anim.add(t);
  Canvas* cv = nullptr; for (int i = 0; i < 24; ++i) { if (i == 8) e.dispatch({Event::Move, {10 + 5 + 44 + 4 + 1 + 4 + 21, 900 - 10 - 52 + 26}}); cv = &e.frame(1.f / 60, [&](Painter& p) { wallpaper(p, e.th); }); }
  cv->savePPM(file); std::printf("écrit %s\n", file);
}
int main() { render(false, "desktop_light.ppm"); render(true, "desktop_dark.ppm"); }
