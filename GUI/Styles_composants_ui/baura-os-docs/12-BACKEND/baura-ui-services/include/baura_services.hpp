// BAURA OS — baura_services.hpp : enveloppe C++ RAII du backend Rust (copie les chaînes, pas de fuite, non copiable).
#ifndef BAURA_SERVICES_HPP
#define BAURA_SERVICES_HPP
#include "baura_ui_services.h"
#include <string>
#include <vector>
namespace BauraSvc {
enum class Layout : uint32_t { SplitDock = 0, FloatingPill = 1, FullWidth = 2 };
enum class Impact : uint32_t { Low = 0, Medium = 1, High = 2 };
struct Group { std::string name, icon; float cpu_pct; uint64_t rss_kib; uint32_t processes, threads; Impact impact; };
struct Hit { std::string id, title, icon; int kind; };
class Services {
 public:
  explicit Services(bool host_proc = false, uint64_t seed = 42) : h_(baura_new(host_proc ? 1 : 0, seed)) {}
  ~Services() { baura_free(h_); }
  Services(const Services&) = delete; Services& operator=(const Services&) = delete;
  BauraServices* raw() { return h_; }
  // préférences
  Layout layout() { return Layout(baura_prefs_get_layout(h_)); }
  bool set_layout(Layout l) { return baura_prefs_set_layout(h_, uint32_t(l)) == 1; }
  uint32_t theme() { return baura_prefs_get_theme(h_); }
  bool set_theme(uint32_t t) { return baura_prefs_set_theme(h_, t) == 1; }
  bool original_icons() { return baura_prefs_get_icon_theme(h_) == 1; }
  bool rtl() { return baura_prefs_is_rtl(h_) != 0; }
  bool set_language(const std::string& l) { return baura_prefs_set_language(h_, l.c_str()) == 1; }
  uint64_t prefs_version() { return baura_prefs_version(h_); }
  std::string serialize_prefs() { return str_([&](char* b, size_t c) { return baura_prefs_serialize(h_, b, c); }); }
  int load_prefs(const std::string& t) { return baura_prefs_load(h_, t.c_str()); }
  // moniteur
  void tick(uint64_t now_ms) { baura_monitor_tick(h_, now_ms); }
  BauraSnapshot snapshot() { BauraSnapshot s{}; baura_monitor_snapshot(h_, &s); return s; }
  std::vector<float> cores() { std::vector<float> v; BauraSnapshot s = snapshot(); for (uint32_t i = 0; i < s.cores; ++i) v.push_back(baura_monitor_core(h_, i)); return v; }
  std::vector<float> history(uint32_t kind) { std::vector<float> v(256); size_t n = baura_monitor_history(h_, kind, v.data(), v.size()); v.resize(n < v.size() ? n : v.size()); return v; }
  std::vector<Group> groups() {
    std::vector<Group> out; size_t n = baura_monitor_group_count(h_);
    for (size_t i = 0; i < n; ++i) { char nm[96] = {0}, ic[48] = {0}; BauraGroup g{}; if (baura_monitor_group(h_, i, &g, nm, sizeof nm, ic, sizeof ic) == 0) out.push_back({nm, ic, g.cpu_pct, g.rss_kib, g.processes, g.threads, Impact(g.impact)}); }
    return out;
  }
  int terminate(const std::string& group) { return baura_monitor_terminate(h_, group.c_str()); }
  // recherche / traduction
  std::vector<Hit> search(const std::string& q, size_t limit = 8) {
    std::vector<Hit> out; size_t n = baura_search(h_, q.c_str(), limit);
    for (size_t i = 0; i < n; ++i) { char id[64] = {0}, t[128] = {0}, ic[48] = {0}; int k = baura_search_hit(h_, i, id, sizeof id, t, sizeof t, ic, sizeof ic); if (k >= 0) out.push_back({id, t, ic, k}); }
    return out;
  }
  std::string tr(const std::string& key) { return str_([&](char* b, size_t c) { return baura_tr(h_, key.c_str(), b, c); }); }
  std::string tr_n(const std::string& key, uint64_t n) { return str_([&](char* b, size_t c) { return baura_tr_n(h_, key.c_str(), n, b, c); }); }
 private:
  template <class F> static std::string str_(F f) { std::string s(256, '\0'); size_t need = f(&s[0], s.size()); if (need >= s.size()) { s.assign(need + 1, '\0'); f(&s[0], s.size()); } s.resize(need); return s; }
  BauraServices* h_;
};
}  // namespace BauraSvc
#endif
