//! Moniteur système : sources de mesures (synthétique déterministe pour tests/démo, /proc Linux pour l'hôte, futur pilote BAURA Core),
//! historique en anneau, regroupement des processus par application et classement d'IMPACT (Faible / Moyen / Élevé).
use std::collections::BTreeMap;
use std::collections::VecDeque;

#[derive(Clone, Debug, Default, PartialEq)]
pub struct ProcSample { pub pid: u32, pub name: String, pub group: String, pub cpu_pct: f32, pub rss_kib: u64, pub threads: u32 }
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Snapshot {
    pub t_ms: u64, pub cpu_total_pct: f32, pub cpu_cores_pct: Vec<f32>, pub mem_total_kib: u64, pub mem_used_kib: u64, pub swap_total_kib: u64, pub swap_used_kib: u64,
    pub net_rx_bps: u64, pub net_tx_bps: u64, pub disk_read_bps: u64, pub disk_write_bps: u64, pub battery_pct: Option<f32>, pub temp_c: Option<f32>, pub procs: Vec<ProcSample>,
}
pub trait Source: Send { fn name(&self) -> &'static str; fn sample(&mut self, now_ms: u64) -> Snapshot; }

// ------------------------------------------------------------------ source synthétique (déterministe)
pub struct Synthetic { seed: u64, cores: usize }
impl Synthetic {
    pub fn new(seed: u64) -> Self { Synthetic { seed, cores: 8 } }
    fn rnd(&self, k: u64) -> f32 { let mut x = self.seed ^ k.wrapping_mul(0x9E3779B97F4A7C15); x ^= x >> 33; x = x.wrapping_mul(0xFF51AFD7ED558CCD); x ^= x >> 33; (x % 10_000) as f32 / 10_000.0 }
    fn wave(&self, t: u64, period: f32, k: u64) -> f32 { let w = ((t as f32 / 1000.0) * std::f32::consts::TAU / period + self.rnd(k) * 6.28).sin() * 0.5 + 0.5; w * 0.75 + self.rnd(t / 1000 * 31 + k) * 0.25 }
}
// (pid, nom du processus, application, charge CPU max %, mémoire de base MiB, fils)
const TABLE: &[(u32, &str, &str, f32, u64, u32)] = &[
    (1, "baura-core", "Système", 3.0, 96, 12), (20, "baura-compositor", "Système", 9.0, 210, 14), (31, "baura-vfs", "Système", 2.0, 48, 6),
    (112, "houb", "HOUB (navigateur)", 38.0, 420, 31), (113, "houb-renderer", "HOUB (navigateur)", 24.0, 780, 18), (114, "houb-renderer", "HOUB (navigateur)", 14.0, 540, 12),
    (140, "bmedia", "BMedia", 7.0, 190, 9), (151, "files", "Fichiers", 2.0, 130, 7), (162, "terminal", "Terminal", 1.5, 62, 3), (175, "code", "Éditeur de code", 18.0, 640, 22),
    (176, "code-helper", "Éditeur de code", 11.0, 380, 10), (190, "monitor", "Moniteur", 2.5, 74, 5), (201, "photos", "Photos", 6.0, 310, 8), (230, "updater", "Mise à jour", 5.0, 88, 4),
];
impl Source for Synthetic {
    fn name(&self) -> &'static str { "synthetic" }
    fn sample(&mut self, now: u64) -> Snapshot {
        let cores: Vec<f32> = (0..self.cores).map(|c| (self.wave(now, 7.0 + c as f32, c as u64 + 1) * 70.0 + 6.0).min(100.0)).collect();
        let procs: Vec<ProcSample> = TABLE.iter().map(|&(pid, n, g, cpu, mib, th)| ProcSample { pid, name: n.into(), group: g.into(), cpu_pct: self.wave(now, 5.0 + (pid % 7) as f32, pid as u64) * cpu, rss_kib: mib * 1024 + (self.rnd(pid as u64 + now / 4000) * 20_000.0) as u64, threads: th }).collect();
        let used: u64 = procs.iter().map(|p| p.rss_kib).sum::<u64>() + 1_900_000;
        Snapshot { t_ms: now, cpu_total_pct: cores.iter().sum::<f32>() / cores.len() as f32, cpu_cores_pct: cores, mem_total_kib: 16 * 1024 * 1024, mem_used_kib: used, swap_total_kib: 4 * 1024 * 1024, swap_used_kib: 120_000,
            net_rx_bps: (self.wave(now, 9.0, 91) * 4_000_000.0) as u64, net_tx_bps: (self.wave(now, 11.0, 92) * 600_000.0) as u64, disk_read_bps: (self.wave(now, 13.0, 93) * 90_000_000.0) as u64, disk_write_bps: (self.wave(now, 6.0, 94) * 30_000_000.0) as u64,
            battery_pct: Some(82.0 - (now as f32 / 60_000.0) % 40.0), temp_c: Some(44.0 + self.wave(now, 20.0, 95) * 16.0), procs }
    }
}

// ------------------------------------------------------------------ /proc Linux (hôte de développement)
#[derive(Default)]
pub struct LinuxProc { prev_cpu: Vec<(u64, u64)>, prev_proc: BTreeMap<u32, u64>, prev_t: u64, prev_total_jiffies: u64, prev_net: (u64, u64), prev_disk: (u64, u64) }
impl LinuxProc { pub fn new() -> Self { Self::default() } pub fn available() -> bool { std::path::Path::new("/proc/stat").exists() } }
fn read(p: &str) -> String { std::fs::read_to_string(p).unwrap_or_default() }
/// Nom d'application déduit d'un nom de processus : on retire les suffixes d'aide (-helper, -renderer, :disk$0…) et les chiffres finaux.
pub fn group_of(comm: &str) -> String {
    let mut s = comm.split(['/', ':']).next().unwrap_or(comm).to_string();
    for suf in ["-helper", "-renderer", "-worker", "-bin", "-wrapped", "_helper"] { if let Some(x) = s.strip_suffix(suf) { s = x.to_string(); } }
    s.trim_end_matches(|c: char| c.is_ascii_digit() || c == '-').to_string()
}
impl Source for LinuxProc {
    fn name(&self) -> &'static str { "linux-proc" }
    fn sample(&mut self, now: u64) -> Snapshot {
        let mut snap = Snapshot { t_ms: now, ..Default::default() };
        let stat = read("/proc/stat"); let mut cur = Vec::new();
        for l in stat.lines().filter(|l| l.starts_with("cpu")) {
            let v: Vec<u64> = l.split_whitespace().skip(1).filter_map(|x| x.parse().ok()).collect(); if v.len() < 4 { continue; }
            cur.push((v.iter().sum::<u64>(), v[3] + v.get(4).copied().unwrap_or(0)));
        }
        let total_now = cur.first().map(|c| c.0).unwrap_or(0);
        let pct = |a: (u64, u64), b: (u64, u64)| { let dt = a.0.saturating_sub(b.0); if dt == 0 { 0.0 } else { 100.0 * (1.0 - a.1.saturating_sub(b.1) as f32 / dt as f32) } };
        if self.prev_cpu.len() == cur.len() && !cur.is_empty() { snap.cpu_total_pct = pct(cur[0], self.prev_cpu[0]); snap.cpu_cores_pct = (1..cur.len()).map(|i| pct(cur[i], self.prev_cpu[i])).collect(); } else { snap.cpu_cores_pct = vec![0.0; cur.len().saturating_sub(1)]; }
        let djiff = total_now.saturating_sub(self.prev_total_jiffies).max(1); self.prev_cpu = cur;
        let mi = read("/proc/meminfo"); let g = |k: &str| mi.lines().find(|l| l.starts_with(k)).and_then(|l| l.split_whitespace().nth(1)).and_then(|x| x.parse::<u64>().ok()).unwrap_or(0);
        snap.mem_total_kib = g("MemTotal:"); snap.mem_used_kib = snap.mem_total_kib.saturating_sub(g("MemAvailable:")); snap.swap_total_kib = g("SwapTotal:"); snap.swap_used_kib = snap.swap_total_kib.saturating_sub(g("SwapFree:"));
        let dt = now.saturating_sub(self.prev_t).max(1) as f64 / 1000.0;
        let (mut rx, mut tx) = (0u64, 0u64);
        for l in read("/proc/net/dev").lines().skip(2) { let (n, r) = match l.split_once(':') { Some(x) => x, None => continue }; if n.trim() == "lo" { continue; } let f: Vec<u64> = r.split_whitespace().filter_map(|x| x.parse().ok()).collect(); if f.len() > 8 { rx += f[0]; tx += f[8]; } }
        if self.prev_t > 0 { snap.net_rx_bps = (rx.saturating_sub(self.prev_net.0) as f64 / dt) as u64; snap.net_tx_bps = (tx.saturating_sub(self.prev_net.1) as f64 / dt) as u64; } self.prev_net = (rx, tx);
        let (mut dr, mut dw) = (0u64, 0u64); for l in read("/proc/diskstats").lines() { let f: Vec<&str> = l.split_whitespace().collect(); if f.len() > 9 && (f[2].starts_with("sd") || f[2].starts_with("nvme") || f[2].starts_with("vd")) && !f[2].chars().last().map_or(false, |c| c.is_ascii_digit() && !f[2].starts_with("nvme")) { dr += f[5].parse::<u64>().unwrap_or(0) * 512; dw += f[9].parse::<u64>().unwrap_or(0) * 512; } }
        if self.prev_t > 0 { snap.disk_read_bps = (dr.saturating_sub(self.prev_disk.0) as f64 / dt) as u64; snap.disk_write_bps = (dw.saturating_sub(self.prev_disk.1) as f64 / dt) as u64; } self.prev_disk = (dr, dw);
        let ncpu = snap.cpu_cores_pct.len().max(1) as f32; let mut next = BTreeMap::new();
        if let Ok(rd) = std::fs::read_dir("/proc") { for e in rd.flatten() {
            let Some(pid) = e.file_name().to_str().and_then(|s| s.parse::<u32>().ok()) else { continue };
            let st = read(&format!("/proc/{pid}/stat")); let (Some(a), Some(b)) = (st.find('('), st.rfind(')')) else { continue };
            let comm = st[a + 1..b].to_string(); let f: Vec<&str> = st[b + 2..].split_whitespace().collect(); if f.len() < 22 { continue; }
            let jiff: u64 = f[11].parse::<u64>().unwrap_or(0) + f[12].parse::<u64>().unwrap_or(0); next.insert(pid, jiff);
            let cpu = match self.prev_proc.get(&pid) { Some(&p) if self.prev_t > 0 => 100.0 * ncpu * jiff.saturating_sub(p) as f32 / djiff as f32, _ => 0.0 };
            let rss = read(&format!("/proc/{pid}/statm")).split_whitespace().nth(1).and_then(|x| x.parse::<u64>().ok()).unwrap_or(0) * 4;
            snap.procs.push(ProcSample { pid, group: group_of(&comm), name: comm, cpu_pct: cpu, rss_kib: rss, threads: f[17].parse().unwrap_or(1) });
        } }
        self.prev_proc = next; self.prev_t = now; self.prev_total_jiffies = total_now; snap
    }
}

// ------------------------------------------------------------------ analyse
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum Impact { Low = 0, Medium = 1, High = 2 }
pub fn impact(cpu_pct: f32, rss_kib: u64, mem_total_kib: u64) -> Impact {
    let mem = if mem_total_kib == 0 { 0.0 } else { 100.0 * rss_kib as f32 / mem_total_kib as f32 };
    if cpu_pct >= 25.0 || mem >= 15.0 { Impact::High } else if cpu_pct >= 8.0 || mem >= 5.0 { Impact::Medium } else { Impact::Low }
}
#[derive(Clone, Debug, PartialEq)]
pub struct AppGroup { pub name: String, pub cpu_pct: f32, pub rss_kib: u64, pub processes: usize, pub threads: u32, pub impact: Impact, pub pids: Vec<u32> }
pub fn group_processes(s: &Snapshot) -> Vec<AppGroup> {
    let mut m: BTreeMap<&str, AppGroup> = BTreeMap::new();
    for p in &s.procs { let g = m.entry(&p.group).or_insert_with(|| AppGroup { name: p.group.clone(), cpu_pct: 0.0, rss_kib: 0, processes: 0, threads: 0, impact: Impact::Low, pids: vec![] }); g.cpu_pct += p.cpu_pct; g.rss_kib += p.rss_kib; g.processes += 1; g.threads += p.threads; g.pids.push(p.pid); }
    let mut v: Vec<AppGroup> = m.into_values().map(|mut g| { g.impact = impact(g.cpu_pct, g.rss_kib, s.mem_total_kib); g }).collect();
    v.sort_by(|a, b| b.impact.cmp(&a.impact).then(b.cpu_pct.partial_cmp(&a.cpu_pct).unwrap_or(std::cmp::Ordering::Equal)).then(b.rss_kib.cmp(&a.rss_kib)).then(a.name.cmp(&b.name))); v
}
/// Historique à capacité fixe (pour les courbes) : le plus ancien disparaît.
#[derive(Clone, Debug)]
pub struct Ring { cap: usize, v: VecDeque<f32> }
impl Ring { pub fn new(cap: usize) -> Self { Ring { cap: cap.max(1), v: VecDeque::new() } } pub fn push(&mut self, x: f32) { if self.v.len() == self.cap { self.v.pop_front(); } self.v.push_back(x); }
    pub fn len(&self) -> usize { self.v.len() } pub fn is_empty(&self) -> bool { self.v.is_empty() } pub fn to_vec(&self) -> Vec<f32> { self.v.iter().copied().collect() } }
/// Terminaison demandée par l'utilisateur : le moniteur ne tue rien lui-même, il demande au noyau via ce canal (voir baura-shell/ffi).
#[derive(Debug, PartialEq, Eq)] pub enum TerminateError { Protected, NotFound }
pub struct Monitor { src: Box<dyn Source>, pub cpu: Ring, pub mem: Ring, pub net: Ring, pub disk: Ring, pub last: Snapshot, pub terminated: Vec<u32> }
impl Monitor {
    pub fn new(src: Box<dyn Source>, history: usize) -> Self { Monitor { src, cpu: Ring::new(history), mem: Ring::new(history), net: Ring::new(history), disk: Ring::new(history), last: Snapshot::default(), terminated: vec![] } }
    pub fn tick(&mut self, now_ms: u64) -> &Snapshot {
        let mut s = self.src.sample(now_ms); s.procs.retain(|p| !self.terminated.contains(&p.pid));
        self.cpu.push(s.cpu_total_pct); self.mem.push(if s.mem_total_kib == 0 { 0.0 } else { 100.0 * s.mem_used_kib as f32 / s.mem_total_kib as f32 });
        self.net.push((s.net_rx_bps + s.net_tx_bps) as f32); self.disk.push((s.disk_read_bps + s.disk_write_bps) as f32); self.last = s; &self.last
    }
    pub fn groups(&self) -> Vec<AppGroup> { group_processes(&self.last) }
    /// Termine tout un groupe (« Terminer » dans l'interface). Les processus système (pid ≤ 50 ou groupe « Système ») sont protégés.
    pub fn terminate_group(&mut self, name: &str) -> Result<usize, TerminateError> {
        if name == "Système" { return Err(TerminateError::Protected); }
        let pids: Vec<u32> = self.last.procs.iter().filter(|p| p.group == name).map(|p| p.pid).collect(); if pids.is_empty() { return Err(TerminateError::NotFound); }
        if pids.iter().any(|&p| p <= 50) { return Err(TerminateError::Protected); }
        self.terminated.extend(&pids); self.last.procs.retain(|p| p.group != name); Ok(pids.len())
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn synthetic_is_deterministic() { let a = Synthetic::new(7).sample(12_345); let b = Synthetic::new(7).sample(12_345); assert_eq!(a, b); assert_ne!(a, Synthetic::new(8).sample(12_345)); }
    #[test] fn values_in_range() { let mut s = Synthetic::new(1); for t in (0..120).map(|i| i * 1000) { let x = s.sample(t); assert!(x.cpu_cores_pct.iter().all(|c| (0.0..=100.0).contains(c))); assert!(x.mem_used_kib < x.mem_total_kib); assert_eq!(x.cpu_cores_pct.len(), 8); } }
    #[test] fn grouping_merges_helpers() { let s = Synthetic::new(1).sample(5000); let g = group_processes(&s); let b = g.iter().find(|g| g.name == "HOUB (navigateur)").unwrap(); assert_eq!(b.processes, 3); assert_eq!(g.iter().map(|g| g.processes).sum::<usize>(), s.procs.len()); }
    #[test] fn grouping_sorted_by_impact_then_cpu() { let g = group_processes(&Synthetic::new(3).sample(9000)); for w in g.windows(2) { assert!(w[0].impact >= w[1].impact); } }
    #[test] fn impact_thresholds() { assert_eq!(impact(1.0, 1000, 16_000_000), Impact::Low); assert_eq!(impact(10.0, 1000, 16_000_000), Impact::Medium); assert_eq!(impact(30.0, 1000, 16_000_000), Impact::High); assert_eq!(impact(1.0, 4_000_000, 16_000_000), Impact::High); }
    #[test] fn ring_caps() { let mut r = Ring::new(3); for i in 0..5 { r.push(i as f32); } assert_eq!(r.to_vec(), vec![2.0, 3.0, 4.0]); }
    #[test] fn group_names() { assert_eq!(group_of("firefox-renderer"), "firefox"); assert_eq!(group_of("Web Content"), "Web Content"); assert_eq!(group_of("kworker/0:1"), "kworker"); }
    #[test] fn terminate_flow() { let mut m = Monitor::new(Box::new(Synthetic::new(1)), 60); m.tick(1000); assert_eq!(m.terminate_group("Système"), Err(TerminateError::Protected)); assert_eq!(m.terminate_group("n'existe pas"), Err(TerminateError::NotFound));
        assert_eq!(m.terminate_group("HOUB (navigateur)"), Ok(3)); m.tick(2000); assert!(m.groups().iter().all(|g| g.name != "HOUB (navigateur)")); assert_eq!(m.cpu.len(), 2); }
    #[test] fn linux_proc_smoke() { if !LinuxProc::available() { return; } let mut l = LinuxProc::new(); let a = l.sample(1000); assert!(a.mem_total_kib > 0 && !a.procs.is_empty()); let b = l.sample(2000); assert_eq!(b.cpu_cores_pct.len(), a.cpu_cores_pct.len()); }
}
