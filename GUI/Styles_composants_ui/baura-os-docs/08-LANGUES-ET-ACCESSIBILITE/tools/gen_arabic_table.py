#!/usr/bin/env python3
"""Génère code/arabic_forms.h : formes de présentation arabes (Unicode FE70–FEFC) tirées de la base Unicode (unicodedata).
Usage : gen_arabic_table.py code/arabic_forms.h"""
import sys, unicodedata
forms = {}   # base -> {iso, fin, ini, med}
lig = {}     # (lam, alef) -> {iso, fin}
for cp in range(0xFE70, 0xFEFD):
    d = unicodedata.decomposition(chr(cp))
    if not d: continue
    parts = d.split(); tag = parts[0].strip('<>'); base = [int(x, 16) for x in parts[1:]]
    key = {'isolated': 'iso', 'final': 'fin', 'initial': 'ini', 'medial': 'med'}.get(tag)
    if not key: continue
    if len(base) == 1: forms.setdefault(base[0], {})[key] = cp
    elif len(base) == 2 and base[0] == 0x644: lig.setdefault(base[1], {})[key] = cp
out = ["// GÉNÉRÉ par tools/gen_arabic_table.py à partir d'unicodedata — ne pas éditer.", "#pragma once", "#include <cstdint>", "namespace BauraText {",
       "struct ArForm { uint16_t base, iso, fin, ini, med; };   // 0 = forme absente ; ini/med présentes => joint des deux côtés",
       "static const ArForm AR_FORMS[] = {"]
for b in sorted(forms):
    f = forms[b]; out.append("  {0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X}," % (b, f.get('iso', 0), f.get('fin', 0), f.get('ini', 0), f.get('med', 0)))
out += ["};", "static const unsigned AR_FORMS_N = sizeof(AR_FORMS) / sizeof(AR_FORMS[0]);",
        "struct ArLig { uint16_t alef, iso, fin; };   // lam + alef (0622, 0623, 0625, 0627)", "static const ArLig AR_LIGS[] = {"]
for a in sorted(lig): out.append("  {0x%04X, 0x%04X, 0x%04X}," % (a, lig[a].get('iso', 0), lig[a].get('fin', 0)))
out += ["};", "static const unsigned AR_LIGS_N = sizeof(AR_LIGS) / sizeof(AR_LIGS[0]);", "}  // namespace BauraText", ""]
open(sys.argv[1], 'w').write("\n".join(out)); print(len(forms), "lettres,", len(lig), "ligatures lam-alef")
