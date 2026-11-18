#!/usr/bin/env python3
"""Cuit une police TTF/OTF (Inter) en atlas binaire BGF1 : bitmaps de couverture 8 bits par taille + crénage.
Aucun analyseur de police n'est nécessaire dans l'OS : il charge ce blob (initrd) et le dessine.
Usage : bake_font.py sortie.bgf Regular.otf SemiBold.otf  [--sizes 12,14,16,20,28,44]
Format BGF1 (petit-boutien) :
  'BGF1' u16 nfaces u16 nsizes u16 nglyphs u16 réservé u32 offset_du_blob
  codepoints[nglyphs] u32 (triés)            -- identiques pour toutes les tailles/graisses
  pour chaque face, pour chaque taille :
     u8 px, i8 ascent, u8 descent, u8 lineheight, u32 off_glyphs, u32 off_kern, u16 nkern, u16 réservé
     glyphes[nglyphs] : u16 advance(1/64 px) i8 bx i8 by u8 w u8 h u32 off_bitmap(relatif au blob)
     crénage[nkern]   : u16 idxA u16 idxB i16 delta(1/64 px)
  blob de bitmaps (couverture 8 bits, ligne par ligne)"""
import struct, sys
from PIL import Image, ImageDraw, ImageFont
args = [a for a in sys.argv[1:] if not a.startswith('--')]
sizes = [12, 14, 16, 20, 28, 44]
for a in sys.argv[1:]:
    if a.startswith('--sizes'): sizes = [int(x) for x in a.split('=')[1].split(',')]
out, faces = args[0], args[1:]
chars = [chr(c) for c in range(32, 127)] + list("ÀÂÄÇÈÉÊËÎÏÔÖÙÛÜŸàâäçèéêëîïôöùûüÿœŒæÆ«»€°·×÷−–—‘’“”…•→←↑↓✓©®≠≤≥")
for a in sys.argv[1:]:                      # --ranges=0x0600-0x06FF,0xFE70-0xFEFF : jeux supplémentaires (arabe, etc.)
    if a.startswith('--ranges='):
        for r in a.split('=')[1].split(','):
            lo, hi = [int(x, 16) for x in r.split('-')]; chars += [chr(c) for c in range(lo, hi + 1)]
cps = sorted(set(ord(c) for c in chars))
KERN_SET = [c for c in cps if 33 <= c < 127]
blob = bytearray(); table = bytearray(); head_sizes = []
def bake(face_path, px):
    font = ImageFont.truetype(face_path, px, layout_engine=ImageFont.Layout.RAQM)
    asc, desc = font.getmetrics(); glyphs = []
    for cp in cps:
        ch = chr(cp); adv = font.getlength(ch); pad = px
        im = Image.new('L', (px * 3, px * 3), 0); ImageDraw.Draw(im).text((pad, pad + asc), ch, font=font, fill=255, anchor='ls')
        bb = im.getbbox()
        if bb is None: glyphs.append((int(adv * 64 + .5), 0, 0, 0, 0, 0)); continue
        crop = im.crop(bb); off = len(blob); blob.extend(crop.tobytes())
        glyphs.append((int(adv * 64 + .5), bb[0] - pad, bb[1] - (pad + asc), crop.width, crop.height, off))
    kern = []; idx = {cp: i for i, cp in enumerate(cps)}
    for a in KERN_SET:
        for b in KERN_SET:
            d = font.getlength(chr(a) + chr(b)) - font.getlength(chr(a)) - font.getlength(chr(b))
            if abs(d) >= 1 / 64: kern.append((idx[a], idx[b], int(round(d * 64))))
    return asc, desc, glyphs, kern
records = []
for f in faces:
    for px in sizes: records.append((px,) + bake(f, px))
nglyph, nkern_total = len(cps), 0
# disposition : en-tête, codepoints, entêtes de tailles, tables, blob
hdr_len = 4 + 8 + 4 + nglyph * 4; sz_len = 16 * len(records); pos = hdr_len + sz_len
tabs = bytearray(); size_heads = bytearray()
for px, asc, desc, glyphs, kern in records:
    og = pos + len(tabs)
    for adv, bx, by, w, h, off in glyphs: tabs += struct.pack('<HbbBBI', adv, bx, by, w, h, off)
    ok = pos + len(tabs)
    for a, b, d in kern: tabs += struct.pack('<HHh', a, b, d)
    size_heads += struct.pack('<BbBBII', px, asc, desc, asc + desc + 2, og, ok) + struct.pack('<HH', len(kern), 0)
blob_base = pos + len(tabs)
# les offsets de bitmap sont relatifs au blob : on les conserve tels quels
with open(out, 'wb') as fh:
    fh.write(b'BGF1' + struct.pack('<HHHHI', len(faces), len(sizes), nglyph, 0, blob_base))
    fh.write(b''.join(struct.pack('<I', c) for c in cps)); fh.write(size_heads); fh.write(tabs); fh.write(blob)
print('écrit', out, len(cps), 'glyphes,', len(records), 'jeux,', (blob_base + len(blob)) // 1024, 'Kio, crénage:', sum(len(r[4]) for r in records), 'paires')
