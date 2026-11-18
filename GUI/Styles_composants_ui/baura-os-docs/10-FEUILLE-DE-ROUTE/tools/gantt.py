#!/usr/bin/env python3
"""Planning de la feuille de route : calcule le calendrier au plus tôt (parallélisme illimité) et dessine render/feuille-de-route.png.
Les durées sont des ESTIMATIONS en semaines-personne (incertitude ±50 %). Usage : gantt.py render/feuille-de-route.png"""
import sys
from PIL import Image, ImageDraw, ImageFont
P = [("P0", "Fondations et règles", .5, []), ("P1", "Identité : thème, marque, icônes", 1, ["P0"]), ("P2", "Texte Inter", 1, ["P0"]), ("P3", "Verre et ombres", 1.5, ["P0"]),
     ("P4", "Composants", 3, ["P1", "P2", "P3"]), ("P5", "Mouvement", 1.5, ["P4"]), ("P6", "Coque : bureau, barre, menus", 3, ["P4"]), ("P7", "Connexion et installateur", 2, ["P4"]),
     ("P8", "12 applications", 6, ["P6"]), ("P9", "Démarrage et framebuffer", 2, ["P1", "P2"]), ("P10", "Langues et accessibilité", 3, ["P4"]), ("P11", "Performance", 3, ["P6"])]
start = {}; end = {}
for k, n, d, deps in P: start[k] = max([end[x] for x in deps] or [0]); end[k] = start[k] + d
total = max(end.values()); effort = sum(d for _, _, d, _ in P)
W, H = 1500, 120 + 46 * len(P); im = Image.new('RGB', (W, H), '#F7F3FF'); g = ImageDraw.Draw(im)
f = ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-Regular.otf', 15); fb = ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-SemiBold.otf', 15); ft = ImageFont.truetype('/usr/share/fonts/opentype/inter/Inter-SemiBold.otf', 22)
g.text((30, 24), "Feuille de route BAURA OS — de l'état du dépôt à la cible", font=ft, fill='#17133D')
g.text((30, 56), f"Chemin critique ≈ {total:.1f} semaines (parallélisme illimité) · effort cumulé ≈ {effort:.1f} semaines-personne · estimations ±50 %", font=f, fill='#5F5A88')
x0, x1 = 360, W - 40; sc = (x1 - x0) / (total + .5)
for w in range(int(total) + 2):
    x = x0 + w * sc; g.line([(x, 96), (x, H - 20)], fill='#DCD3FF'); g.text((x + 4, 88), f"S{w}", font=f, fill='#5F5A88')
cols = {"P0": '#8B5CF6', "P1": '#4F46E5', "P2": '#4F46E5', "P3": '#4F46E5', "P4": '#6D66F2', "P5": '#6D66F2', "P6": '#6D66F2', "P7": '#AB8E18', "P8": '#AB8E18', "P9": '#34A37A', "P10": '#34A37A', "P11": '#D6303C'}
for i, (k, n, d, deps) in enumerate(P):
    y = 120 + i * 46; g.text((30, y + 6), f"{k}  {n}", font=fb, fill='#17133D'); a, b = x0 + start[k] * sc, x0 + end[k] * sc
    g.rounded_rectangle([a, y, b, y + 28], radius=10, fill=cols[k]); g.text((a + 10, y + 5) if b - a > 70 else (b + 8, y + 5), f"{d:g} sem", font=f, fill='white' if b - a > 70 else '#17133D')
    for dpk in deps:
        j = [p[0] for p in P].index(dpk); g.line([(x0 + end[dpk] * sc, 120 + j * 46 + 14), (a, y + 14)], fill='#8B5CF6', width=1)
im.save(sys.argv[1]); print("total", total, "effort", effort)
for k, n, d, deps in P: print(k, start[k], end[k])
