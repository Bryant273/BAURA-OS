// Intégration C++ ↔ Rust : le C++ appelle la bibliothèque statique par l'ABI C (via l'enveloppe RAII).
#include <cstdio>
#include "baura_services.hpp"
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("ECHEC: %s (ligne %d)\n", #c, __LINE__); ++fails; } } while (0)
using namespace BauraSvc;
int main() {
  Services s;
  CHECK(s.layout() == Layout::SplitDock);                       // le bureau PAR DÉFAUT est la barre coupée en deux
  CHECK(s.set_layout(Layout::FloatingPill) && !s.set_layout(Layout::FloatingPill) && s.layout() == Layout::FloatingPill);
  uint64_t v0 = s.prefs_version(); std::string saved = s.serialize_prefs(); CHECK(saved.find("layout=floating-pill") != std::string::npos);
  Services t; CHECK(t.load_prefs(saved) == 0 && t.layout() == Layout::FloatingPill && t.prefs_version() > 0); (void)v0;
  CHECK(t.load_prefs("layout=nimporte\n") == 1);
  for (int i = 1; i <= 30; ++i) s.tick(i * 1000);
  BauraSnapshot sn = s.snapshot(); CHECK(sn.cores == 8 && sn.mem_total_kib == 16u * 1024 * 1024 && sn.cpu_total_pct >= 0 && sn.cpu_total_pct <= 100);
  CHECK(s.history(0).size() == 30 && s.cores().size() == 8);
  auto g = s.groups(); CHECK(g.size() > 8); bool sorted = true; for (size_t i = 1; i < g.size(); ++i) if (g[i - 1].impact < g[i].impact) sorted = false; CHECK(sorted);
  bool has_icon = true; for (auto& x : g) if (x.icon.empty()) has_icon = false; CHECK(has_icon);
  CHECK(s.terminate("Système") == 1 && s.terminate("Fantôme") == 2 && s.terminate("Photos") == 0);
  bool gone = true; for (auto& x : s.groups()) if (x.name == "Photos") gone = false; CHECK(gone);
  auto hits = s.search("term"); CHECK(!hits.empty() && hits[0].id == "terminal" && hits[0].icon == "terminal");
  CHECK(s.tr("start.title") == "Démarrer"); CHECK(s.tr_n("notif.count", 3) == "3 notifications");
  s.set_language("ar"); CHECK(s.rtl() && s.tr("start.title") == "ابدأ" && s.tr_n("notif.count", 2) == "إشعاران");
  char icon[48]; baura_icon_resolve("org.gnome.Nautilus", "", icon, sizeof icon); CHECK(std::string(icon) == "files");
  Services host(true); host.tick(1000); host.tick(2000); CHECK(host.snapshot().mem_total_kib > 0 && !host.groups().empty());   // /proc réel (ou synthétique si absent)
  std::printf("%d échec(s)\n", fails); return fails ? 1 : 0;
}
