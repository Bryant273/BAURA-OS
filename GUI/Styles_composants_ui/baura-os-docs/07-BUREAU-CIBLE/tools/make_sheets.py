#!/usr/bin/env python3
"""Planches de contact de 07 : ecrans_planche.png (coque + installateur) et apps_planche.png (12 applications). Usage : make_sheets.py ../render"""
import glob, os, sys
from PIL import Image
r = sys.argv[1]
def sheet(files, cols, W, H, out):
    rows = (len(files) + cols - 1) // cols; s = Image.new('RGB', (W * cols, H * rows))
    for i, f in enumerate(files): s.paste(Image.open(f).resize((W, H), Image.LANCZOS), ((i % cols) * W, (i // cols) * H))
    s.save(out)
sheet(sorted(glob.glob(os.path.join(r, 'app_*.png'))), 3, 640, 400, os.path.join(r, 'apps_planche.png'))
sheet([os.path.join(r, n + '.png') for n in ['bureau_defaut_clair', 'bureau_defaut_sombre', 'demarrer_clair', 'recherche_sombre', 'centre-actions_sombre', 'connexion_sombre', 'uac_sombre', 'bulle-et-menu_sombre', 'installateur_1', 'installateur_2', 'installateur_3', 'installateur_4']], 3, 640, 360, os.path.join(r, 'ecrans_planche.png'))
