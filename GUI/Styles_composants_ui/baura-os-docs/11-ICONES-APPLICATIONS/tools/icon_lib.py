"""Bibliothèque de dessin des icônes d'applications BAURA (grille 64×64). Tous les dessins sont ORIGINAUX (aucun fichier Papirus ni logo de marque copié).
Langage visuel : tuile à coins arrondis en dégradé vertical · motifs blancs (deux tons) · ombre portée douce sous le motif principal ·
un petit POINT DORÉ (clin d'œil au symbole BAURA) dans chaque icône. Les dossiers et types de fichiers sont autonomes (sans tuile), comme dans Papirus."""
import math
# ---------- couleurs ----------
def rgba(h, a=1.0):
    h = h.lstrip('#'); r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16); al = int(round(a * 255)); return (al << 24) | (r << 16) | (g << 8) | b
WHITE, BLACK, GOLD = '#FFFFFF', '#000000', '#E5C34A'
TILES = {'indigo': ('#6D66F2', '#4338CA'), 'violet': ('#A78BFA', '#6D28D9'), 'blue': ('#60A5FA', '#1D4ED8'), 'sky': ('#38BDF8', '#0369A1'), 'teal': ('#2DD4BF', '#0F766E'),
         'green': ('#4ADE80', '#047857'), 'lime': ('#BEF264', '#4D7C0F'), 'amber': ('#FCD34D', '#D97706'), 'orange': ('#FDBA74', '#EA580C'), 'red': ('#FCA5A5', '#B91C1C'),
         'rose': ('#FDA4AF', '#BE123C'), 'pink': ('#F9A8D4', '#BE185D'), 'slate': ('#A5B4C8', '#475569'), 'dark': ('#52607A', '#0F172A'), 'gray': ('#B4BAE0', '#6B7199'),
         'night': ('#4C46B8', '#1E1B4B'), 'gold': ('#F2D66B', '#AB8E18'), 'cyan': ('#67E8F9', '#0E7490'), 'fuchsia': ('#F0ABFC', '#A21CAF')}
# ---------- éléments ----------
def E(d, fill=None, grad=None, stroke=None, sw=0, sh=False, m=0, angle=180):
    """d : trajet SVG ; fill : '#RRGGBB' ou (couleur, alpha) ; grad : ((c0,a0),(c1,a1)) ; stroke : couleur ; sh : ombre ; m : taille minimale d'affichage (px)."""
    return dict(d=d, fill=fill, grad=grad, stroke=stroke, sw=sw, sh=sh, m=m, angle=angle)
def f(c, a=1.0): return (c, a)
def n(v): return ('%.2f' % v).rstrip('0').rstrip('.')
def rr(x, y, w, h, r):
    r = min(r, w / 2, h / 2); return f"M{n(x+r)} {n(y)}H{n(x+w-r)}A{n(r)} {n(r)} 0 0 1 {n(x+w)} {n(y+r)}V{n(y+h-r)}A{n(r)} {n(r)} 0 0 1 {n(x+w-r)} {n(y+h)}H{n(x+r)}A{n(r)} {n(r)} 0 0 1 {n(x)} {n(y+h-r)}V{n(y+r)}A{n(r)} {n(r)} 0 0 1 {n(x+r)} {n(y)}Z"
def circ(cx, cy, r, rev=False):
    s = 0 if rev else 1; return f"M{n(cx-r)} {n(cy)}A{n(r)} {n(r)} 0 1 {s} {n(cx+r)} {n(cy)}A{n(r)} {n(r)} 0 1 {s} {n(cx-r)} {n(cy)}Z"
def ell(cx, cy, rx, ry): return f"M{n(cx-rx)} {n(cy)}A{n(rx)} {n(ry)} 0 1 1 {n(cx+rx)} {n(cy)}A{n(rx)} {n(ry)} 0 1 1 {n(cx-rx)} {n(cy)}Z"
def poly(pts, close=True): return "M" + "L".join(f"{n(x)} {n(y)}" for x, y in pts) + ("Z" if close else "")
def gear(cx, cy, ro, ri, teeth=8, hole=0):
    pts = []
    for i in range(teeth):
        a = 2 * math.pi * i / teeth; w = math.pi / teeth
        for da, r in ((-.55 * w, ri), (-.30 * w, ro), (.30 * w, ro), (.55 * w, ri)): pts.append((cx + r * math.cos(a + da), cy + r * math.sin(a + da)))
    d = poly(pts); return d + (circ(cx, cy, hole, True) if hole else '')
def star4(cx, cy, r, k=.28):
    pts = []
    for i in range(8): a = math.pi / 4 * i - math.pi / 2; rr_ = r if i % 2 == 0 else r * k; pts.append((cx + rr_ * math.cos(a), cy + rr_ * math.sin(a)))
    return poly(pts)
def xf(d, s, tx, ty):   # transforme un trajet absolu (M L H V A Z) : x' = x*s+tx, y' = y*s+ty
    import re; out = []; tok = re.findall(r'[MLHVAZ]|-?\d*\.?\d+', d); i = 0; cmd = None
    while i < len(tok):
        t = tok[i]
        if t in 'MLHVAZ': cmd = t; out.append(t); i += 1; continue
        if cmd in 'ML': out.append(n(float(tok[i]) * s + tx) + ' ' + n(float(tok[i + 1]) * s + ty)); i += 2
        elif cmd == 'H': out.append(n(float(t) * s + tx)); i += 1
        elif cmd == 'V': out.append(n(float(t) * s + ty)); i += 1
        elif cmd == 'A': v = tok[i:i + 7]; out.append(f"{n(float(v[0])*s)} {n(float(v[1])*s)} {v[2]} {v[3]} {v[4]} {n(float(v[5])*s+tx)} {n(float(v[6])*s+ty)}"); i += 7
    return ''.join(out)
BAR = "M174 29A24 24 0 0 1 198 53V425A24 24 0 0 1 150 425V53A24 24 0 0 1 174 29Z"; TOP = "M248 29A101 102.5 0 0 1 248 234Z"; BOT = "M248 245A101 102 0 0 1 248 449Z"; DOT = "M331 221H334A15 15 0 0 1 349 236V242A15 15 0 0 1 334 257H331A15 15 0 0 1 316 242V236A15 15 0 0 1 331 221Z"
def symbol(x, y, h, c=(WHITE, 1), c2=(WHITE, .85), c3=(WHITE, .65), c4=(GOLD, 1), sh=False):   # symbole BAURA dans la boîte (x,y,h) ; largeur = h*199/420
    s = h / 420.0; tx, ty = x - 150 * s, y - 29 * s
    return [E(xf(BAR, s, tx, ty), c, sh=sh), E(xf(TOP, s, tx, ty), c2), E(xf(BOT, s, tx, ty), c3), E(xf(DOT, s, tx, ty), c4)]
def dot(x, y, r=2.8, c=GOLD): return E(circ(x, y, r), f(c))
def sh(e):
    e = dict(e); e['sh'] = True; return e
def line(pts, c=WHITE, w=3, a=1.0, m=0): return E(poly(pts, False), None, stroke=f(c, a), sw=w, m=m)
def pth(d, c=WHITE, w=3, a=1.0, m=0): return E(d, None, stroke=f(c, a), sw=w, m=m)
def W(d, a=1.0, s=False, m=0): return E(d, f(WHITE, a), sh=s, m=m)
def C(d, c, a=1.0, s=False, m=0): return E(d, f(c, a), sh=s, m=m)
def G(d, c0, c1, a=1.0, s=False, angle=180, m=0): return E(d, None, grad=((c0, a), (c1, a)), sh=s, angle=angle, m=m)
ICONS = {}
def icon(id, name, cat, bg, elems, alias=(), standalone=False):
    ICONS[id] = dict(id=id, name=name, cat=cat, bg=None if standalone else TILES[bg], elems=[e for e in elems], alias=list(alias))
