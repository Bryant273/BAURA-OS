#!/usr/bin/env python3
"""Convertit tous les *.ppm du dossier courant en PNG dans le dossier donné, puis supprime les .ppm.
Usage : ppm2png.py ../render [ancien=nouveau ...]   (renommage optionnel, sans extension)"""
import glob, os, sys
from PIL import Image
out = sys.argv[1]; ren = dict(a.split('=') for a in sys.argv[2:]); os.makedirs(out, exist_ok=True)
for f in sorted(glob.glob('*.ppm')):
    n = f[:-4]; n = ren.get(n, n); Image.open(f).save(os.path.join(out, n + '.png')); os.remove(f)
