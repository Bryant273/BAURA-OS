#!/usr/bin/env python3
"""Génère à partir des dessins Python (icons_*.py) : ../../common/appicons_data.h (C++), ../svg/*.svg, ../catalogue_appicons.json,
et les tables Rust du backend ../../12-BACKEND/baura-ui-services/crates/baura-icons/src/data.rs.  Usage : gen_appicons.py"""
import json, os, sys
sys.path.insert(0, os.path.dirname(__file__))
from icon_lib import ICONS, rgba, WHITE
import icons_system, icons_files  # noqa
import icons_functional           # noqa
from known_apps import KNOWN, CATEGORY_FALLBACK
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
def col(c):   # (hex, alpha) | hex -> ARGB int
    if isinstance(c, tuple): return rgba(c[0], c[1])
    return rgba(c, 1.0)
def css(c):
    h, a = (c if isinstance(c, tuple) else (c, 1.0)); return h if a >= .999 else h + '%02X' % int(round(a * 255))
ids = list(ICONS)
_seen = set(ids)
for _i in ids:   # alias uniques : le premier gagne, jamais un alias qui est déjà un id
    keep = []
    for a in ICONS[_i]['alias']:
        if a not in _seen: _seen.add(a); keep.append(a)
    ICONS[_i]['alias'] = keep
# ---- C++ ----
L = ["// GÉNÉRÉ par 11-ICONES-APPLICATIONS/tools/gen_appicons.py — ne pas éditer à la main.", "// Icônes d'applications BAURA : dessins ORIGINAUX (grille 64×64), inspirés du style des thèmes plats type Papirus — aucun fichier ni logo copié.", "#pragma once", "#include <cstddef>", "#include <cstdint>", "namespace BauraVec::appicons {",
     "struct Elem { const char* d; uint32_t c0, c1; uint16_t angle; uint8_t flags; uint8_t min_px; float sw; uint32_t sc; };   // flags : 1 ombre, 2 dégradé, 4 trait, 8 remplissage",
     "struct Icon { const char* id; const char* name; const char* cat; uint32_t bg0, bg1; bool tile; const Elem* e; std::size_t n; };", "struct Alias { const char* from; std::size_t idx; };"]
for ix, i in enumerate(ids):
    ic = ICONS[i]; rows = []
    for e in ic['elems']:
        fl = (1 if e['sh'] else 0) | (2 if e['grad'] else 0) | (4 if e['stroke'] else 0) | (8 if (e['fill'] or e['grad']) else 0)
        c0 = col(e['grad'][0]) if e['grad'] else (col(e['fill']) if e['fill'] else 0); c1 = col(e['grad'][1]) if e['grad'] else 0
        rows.append('{"%s", 0x%08X, 0x%08X, %d, %d, %d, %sf, 0x%08X}' % (e['d'], c0, c1, e['angle'], fl, e['m'], ('%.2f' % e['sw']), col(e['stroke']) if e['stroke'] else 0))
    L.append("static const Elem e_%d[] = {%s};" % (ix, ",\n  ".join(rows)))
L.append("static const Icon ALL[] = {")
for ix, i in enumerate(ids):
    ic = ICONS[i]; bg = ic['bg']; L.append('  {"%s", "%s", "%s", 0x%08X, 0x%08X, %s, e_%d, %d},' % (i, ic['name'].replace('"', '\\"'), ic['cat'], rgba(bg[0]) if bg else 0, rgba(bg[1]) if bg else 0, 'true' if bg else 'false', ix, len(ic['elems'])))
L.append("};"); L.append("static const std::size_t COUNT = %d;" % len(ids))
al = [(a, ix) for ix, i in enumerate(ids) for a in ICONS[i]['alias']]
L.append("static const Alias ALIASES[] = {" + ",".join('{"%s", %d}' % a for a in al) + "};"); L.append("static const std::size_t ALIAS_COUNT = %d;" % len(al))
# applications connues -> icône (la table est aussi livrée au backend Rust)
kn = [(k, v) for k, v in sorted(KNOWN.items()) if v in ICONS]
L.append("static const Alias KNOWN_APPS[] = {" + ",".join('{"%s", %d}' % (k, ids.index(v)) for k, v in kn) + "};"); L.append("static const std::size_t KNOWN_COUNT = %d;" % len(kn)); L.append("}  // namespace BauraVec::appicons"); L.append("")
open(os.path.join(ROOT, 'common', 'appicons_data.h'), 'w').write("\n".join(L))
# ---- SVG ----
os.makedirs(os.path.join(HERE, '..', 'svg'), exist_ok=True)
for i in ids:
    ic = ICONS[i]; out = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">', '<defs>']; k = 0; body = []
    if ic['bg']: out.append('<linearGradient id="bg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></linearGradient>' % ic['bg'])
    for e in ic['elems']:
        if e['grad']:
            k += 1; out.append('<linearGradient id="g%d" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="%s" stop-opacity="%.2f"/><stop offset="1" stop-color="%s" stop-opacity="%.2f"/></linearGradient>' % (k, e['grad'][0][0], e['grad'][0][1], e['grad'][1][0], e['grad'][1][1])); fill = 'url(#g%d)' % k
        elif e['fill']: fill = e['fill'][0] if isinstance(e['fill'], tuple) else e['fill']
        else: fill = 'none'
        op = (e['fill'][1] if isinstance(e['fill'], tuple) and e['fill'] else 1.0)
        if e['sh'] and fill != 'none': body.append('<path d="%s" fill="#000" fill-opacity=".18" transform="translate(0 1.6)"/>' % e['d'])
        if e['stroke']: body.append('<path d="%s" fill="none" stroke="%s" stroke-opacity="%.2f" stroke-width="%s" stroke-linecap="round" stroke-linejoin="round"/>' % (e['d'], e['stroke'][0], e['stroke'][1], e['sw']))
        else: body.append('<path d="%s" fill="%s"%s/>' % (e['d'], fill, (' fill-opacity="%.2f"' % op) if op < .999 and not e['grad'] else ''))
    out.append('</defs>')
    if ic['bg']: out.append('<rect x="4" y="4" width="56" height="56" rx="15" fill="url(#bg)"/><rect x="4.5" y="4.5" width="55" height="55" rx="14.5" fill="none" stroke="#fff" stroke-opacity=".3"/>')
    out += body; out.append('</svg>'); open(os.path.join(HERE, '..', 'svg', i + '.svg'), 'w').write("\n".join(out))
json.dump({i: dict(name=ICONS[i]['name'], category=ICONS[i]['cat'], alias=ICONS[i]['alias']) for i in ids}, open(os.path.join(HERE, '..', 'catalogue_appicons.json'), 'w'), ensure_ascii=False, indent=1)
# ---- Rust ----
rdir = os.path.join(ROOT, '12-BACKEND', 'baura-ui-services', 'crates', 'baura-icons', 'src'); os.makedirs(rdir, exist_ok=True)
R = ["// GÉNÉRÉ par 11-ICONES-APPLICATIONS/tools/gen_appicons.py — ne pas éditer à la main.", "pub struct IconInfo { pub id: &'static str, pub name: &'static str, pub category: &'static str, pub standalone: bool }", "pub static ICONS: &[IconInfo] = &["]
for i in ids: R.append('    IconInfo { id: "%s", name: "%s", category: "%s", standalone: %s },' % (i, ICONS[i]['name'].replace('"', '\\"'), ICONS[i]['cat'], 'true' if not ICONS[i]['bg'] else 'false'))
R += ["];", "pub static ALIASES: &[(&str, &str)] = &["] + ['    ("%s", "%s"),' % (a, i) for i in ids for a in ICONS[i]['alias']] + ["];", "pub static KNOWN_APPS: &[(&str, &str)] = &["] + ['    ("%s", "%s"),' % kv for kv in kn] + ["];", "pub static CATEGORY_FALLBACK: &[(&str, &str)] = &["] + ['    ("%s", "%s"),' % kv for kv in sorted(CATEGORY_FALLBACK.items()) if kv[1] in ICONS] + ["];", ""]
open(os.path.join(rdir, 'data.rs'), 'w').write("\n".join(R))
print(len(ids), "icônes,", len(al), "alias,", len(kn), "applications connues")
