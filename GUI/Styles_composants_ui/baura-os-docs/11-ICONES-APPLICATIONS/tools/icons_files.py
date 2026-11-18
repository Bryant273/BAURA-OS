"""Dossiers et types de fichiers : icônes AUTONOMES (sans tuile), comme dans les thèmes plats."""
from icon_lib import *
def folder(id, name, c_back, c0, c1, emblem=(), alias=()):
    el = [sh(C("M6 17a5 5 0 0 1 5-5h12l5 5h25a5 5 0 0 1 5 5v26a5 5 0 0 1-5 5H11a5 5 0 0 1-5-5Z", c_back)),
          W(rr(6, 24, 52, 4, 2), .25), sh(G(rr(6, 22, 52, 30, 6), c0, c1))]
    icon(id, name, 'dossiers', None, el + list(emblem), alias, standalone=True)
WHT = lambda d, a=1.0: W(d, a)
folder('folder', 'Dossier', '#4338CA', '#8B84F5', '#5B52E8', [dot(49, 44, 3)], ['folder-default', 'inode-directory', 'folder-blue'])
folder('folder-documents', 'Documents', '#1D4ED8', '#7AB2FB', '#2F6BEA', [W(rr(24, 29, 16, 19, 2.5), .95), *[line([(28, y), (36, y)], '#2F6BEA', 2, .7) for y in (35, 40, 45)]], ['folder-documents'])
folder('folder-pictures', 'Images', '#BE185D', '#F9A8D4', '#EC4899', [C(circ(24, 33, 3.4), GOLD), W("M20 49L30 38L37 45L42 40L50 49Z", .95)], ['folder-pictures', 'folder-image'])
folder('folder-music', 'Musique', '#6D28D9', '#C4B5FD', '#8B5CF6', [W(rr(34, 31, 2.6, 14, 1.3)), W(poly([(34, 31), (44, 29), (44, 33), (34, 35)])), W(circ(32, 46, 3.6)), W(circ(42, 44, 3.6))], ['folder-music', 'folder-sound'])
folder('folder-videos', 'Vidéos', '#B91C1C', '#FCA5A5', '#EF4444', [W(poly([(30, 31), (30, 47), (44, 39)]))], ['folder-videos', 'folder-video'])
folder('folder-downloads', 'Téléchargements', '#047857', '#86EFAC', '#10B981', [W(poly([(28, 33), (36, 33), (36, 39), (41, 39), (32, 48), (23, 39), (28, 39)]))], ['folder-download', 'folder-downloads'])
folder('folder-desktop', 'Bureau', '#0F766E', '#99F6E4', '#14B8A6', [W(rr(22, 30, 20, 13, 2.5)), W(rr(28, 44, 8, 2.4, 1.2))], ['user-desktop', 'folder-desktop'])
folder('folder-home', 'Dossier personnel', '#4338CA', '#A5B4FC', '#6366F1', [W(poly([(32, 29), (43, 38), (40, 38), (40, 48), (24, 48), (24, 38), (21, 38)])), C(rr(29.5, 41, 5, 7, 1.5), '#6366F1')], ['user-home', 'folder-home'])
# types de fichiers : page + coin plié + bandeau coloré + pictogramme
def mime(id, name, band0, band1, glyph, alias=()):
    page = "M14 5H37L51 19V54A5 5 0 0 1 46 59H14A5 5 0 0 1 9 54V10A5 5 0 0 1 14 5Z"
    el = [sh(G(page, '#FFFFFF', '#E8EAF6')), E("M37 5V15A4 4 0 0 0 41 19H51Z", f('#B9BEDD', .9)), G("M9 39H51V54A5 5 0 0 1 46 59H14A5 5 0 0 1 9 54Z", band0, band1)] + glyph
    icon(id, name, 'types de fichiers', None, el, alias, standalone=True)
lines = lambda c: [line([(16, y), (32 if y > 20 else 44, y)], c, 2.4, .55) for y in (18, 25, 32)]
mime('file-text', 'Texte', '#94A3B8', '#475569', lines('#475569') + [line([(18, 49), (42, 49)], '#FFFFFF', 3, .9)], ['text-x-generic', 'text-plain', 'text-x-generic-template'])
mime('file-doc', 'Document', '#60A5FA', '#1D4ED8', lines('#1D4ED8') + [line([(18, 46), (30, 46)], '#FFFFFF', 3, .9), line([(18, 52), (40, 52)], '#FFFFFF', 3, .6)], ['x-office-document', 'application-msword', 'application-vnd.oasis.opendocument.text'])
mime('file-sheet', 'Feuille de calcul', '#4ADE80', '#047857', [*[C(rr(16 + i * 11, 16 + j * 7, 9, 5, 1.2), '#047857', .5) for i in range(3) for j in range(3)], C(rr(16, 44, 28, 3.4, 1.7), '#FFFFFF', .9)], ['x-office-spreadsheet', 'application-vnd.ms-excel', 'application-vnd.oasis.opendocument.spreadsheet'])
mime('file-slides', 'Présentation', '#FDBA74', '#EA580C', [C(rr(16, 14, 28, 17, 2.5), '#EA580C', .4), C(poly([(18, 28), (25, 22), (30, 26), (36, 19), (42, 28)]), '#EA580C', .7), line([(18, 47), (36, 47)], '#FFFFFF', 3, .9)], ['x-office-presentation', 'application-vnd.ms-powerpoint'])
mime('file-pdf', 'PDF', '#FCA5A5', '#B91C1C', lines('#B91C1C') + [line([(18, 49), (40, 49)], '#FFFFFF', 3, .9)], ['application-pdf', 'application-x-pdf'])
mime('file-image', 'Image', '#F9A8D4', '#BE185D', [C(circ(24, 22, 3.4), GOLD), C("M14 36L26 24L33 31L38 27L47 36Z", '#BE185D', .6), line([(18, 49), (36, 49)], '#FFFFFF', 3, .9)], ['image-x-generic', 'image-png', 'image-jpeg'])
mime('file-audio', 'Audio', '#C4B5FD', '#6D28D9', [C(rr(30, 17, 2.8, 14, 1.4), '#6D28D9', .8), C(poly([(30, 17), (40, 15), (40, 19), (30, 21)]), '#6D28D9', .8), C(circ(27, 32, 3.6), '#6D28D9', .8), line([(18, 49), (38, 49)], '#FFFFFF', 3, .9)], ['audio-x-generic', 'audio-mpeg', 'audio-x-wav'])
mime('file-video', 'Vidéo', '#FDA4AF', '#BE123C', [C(rr(15, 15, 30, 20, 3), '#BE123C', .35), C(poly([(26, 20), (26, 30), (35, 25)]), '#BE123C', .85), line([(18, 49), (36, 49)], '#FFFFFF', 3, .9)], ['video-x-generic', 'video-mp4', 'video-x-matroska'])
mime('file-archive', 'Archive', '#FCD34D', '#D97706', [*[C(rr(28, 13 + k * 6, 6, 3.4, 1), '#D97706', .75 if k % 2 else .45) for k in range(4)], C(rr(27, 36, 8, 7, 2.5), '#D97706', .8), line([(18, 49), (40, 49)], '#FFFFFF', 3, .9)], ['package-x-generic', 'application-zip', 'application-x-compressed-tar'])
mime('file-code', 'Code source', '#5EEAD4', '#0F766E', [line([(24, 20), (17, 26.5), (24, 33)], '#0F766E', 3, .85), line([(36, 20), (43, 26.5), (36, 33)], '#0F766E', 3, .85), line([(32, 17), (28, 36)], GOLD, 2.8), line([(18, 49), (38, 49)], '#FFFFFF', 3, .9)], ['text-x-script', 'text-x-source', 'text-x-csrc', 'text-x-python', 'application-x-executable'])
mime('file-unknown', 'Fichier', '#B4BAE0', '#6B7199', [E("M25 23a6 6 0 1 1 9 5c-2 1-3 2-3 4", None, stroke=f('#6B7199'), sw=3.2), dot(31, 36.5, 2.4, '#6B7199'), line([(18, 49), (36, 49)], '#FFFFFF', 3, .9)], ['unknown', 'application-octet-stream', 'text-x-generic-template'])
