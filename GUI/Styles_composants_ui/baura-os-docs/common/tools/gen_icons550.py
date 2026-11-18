#!/usr/bin/env python3
"""Génère common/icons_data.h (550 icônes) = 97 icônes BAURA (baura-icons.js) + 453 icônes Lucide (licence ISC) triées pour l'OS.
Usage : python3 gen_icons550.py /chemin/vers/lucide/icons        (clone : github.com/lucide-icons/lucide, dossier icons/)
Sorties : common/icons_data.h, 09-ICONES/CATALOGUE.md, 09-ICONES/catalogue.json"""
import json, re, subprocess, pathlib, sys, collections
here = pathlib.Path(__file__).resolve().parents[2]; luc = pathlib.Path(sys.argv[1])
# ---- 1. icônes BAURA d'origine -----
js = "global.window={};require('%s');console.log(JSON.stringify(window.BI.paths))" % (here / 'common/baura-icons.js')
mine = json.loads(subprocess.check_output(['node', '-e', js]))
def attrs(s): return dict(re.findall(r'([a-z0-9-]+)="([^"]*)"', s))
def f(v): return ('%.3f' % float(v)).rstrip('0').rstrip('.')
def rect(a):
    x, y, w, h = [float(a[k]) for k in ('x', 'y', 'width', 'height')]; r = min(float(a.get('rx', a.get('ry', 0))), w / 2, h / 2)
    if r == 0: return 'M%s %sh%sv%sh-%sz' % (f(x), f(y), f(w), f(h), f(w))
    return ('M%s %sh%sa%s %s 0 0 1 %s %sv%sa%s %s 0 0 1 -%s %sh-%sa%s %s 0 0 1 -%s -%sv-%sa%s %s 0 0 1 %s -%sz'
            % (f(x + r), f(y), f(w - 2 * r), f(r), f(r), f(r), f(r), f(h - 2 * r), f(r), f(r), f(r), f(r), f(w - 2 * r), f(r), f(r), f(r), f(r), f(h - 2 * r), f(r), f(r), f(r), f(r)))
def circle(a):
    cx, cy, r = [float(a[k]) for k in ('cx', 'cy', 'r')]
    return 'M%s %sa%s %s 0 1 0 %s 0a%s %s 0 1 0 -%s 0z' % (f(cx - r), f(cy), f(r), f(r), f(2 * r), f(r), f(r), f(2 * r))
def ellipse(a):
    cx, cy, rx, ry = [float(a[k]) for k in ('cx', 'cy', 'rx', 'ry')]
    return 'M%s %sa%s %s 0 1 0 %s 0a%s %s 0 1 0 -%s 0z' % (f(cx - rx), f(cy), f(rx), f(ry), f(2 * rx), f(rx), f(ry), f(2 * rx))
def line(a): return 'M%s %sL%s %s' % (f(a['x1']), f(a['y1']), f(a['x2']), f(a['y2']))
def poly(a, close): pts = re.findall(r'-?[\d.]+', a['points']); p = ['%s %s' % (pts[i], pts[i + 1]) for i in range(0, len(pts) - 1, 2)]; return 'M' + 'L'.join(p) + ('z' if close else '')
CONV = {'path': lambda a: a['d'], 'rect': rect, 'circle': circle, 'ellipse': ellipse, 'line': line, 'polyline': lambda a: poly(a, False), 'polygon': lambda a: poly(a, True)}
def elems(svg):
    out = []
    for m in re.finditer(r'<(path|rect|circle|ellipse|line|polyline|polygon)\s([^>]*?)/?>', svg, re.S):
        a = attrs(m.group(2)); out.append((CONV[m.group(1)](a), a.get('fill') == 'currentColor'))
    return out

# anciens noms Lucide / identifiants du catalogue Papirus du dépôt (GUI/UI_FRAMEWORK/papirus_icon_catalog.cpp) -> nom d'icône BAURA
ALIAS = {'trash-2': 'trash', 'circle-help': 'circle-question-mark', 'bar-chart-3': 'chart-column', 'filter': 'funnel', 'history': 'rotate-ccw', 'sidebar': 'panel-left',
  'sidebar-close': 'panel-left-close', 'sidebar-open': 'panel-left-open', 'usb-c': 'usb',
  'folder': 'folder', 'folder-home': 'house', 'folder-download': 'folder-down', 'folder-documents': 'folder', 'folder-pictures': 'images', 'text-x-generic': 'file', 'text-plain': 'file-text',
  'image-x-generic': 'image', 'audio-x-generic': 'music', 'video-x-generic': 'film', 'application-x-executable': 'app-window', 'preferences-system': 'settings', 'network-wireless': 'wifi',
  'network-off': 'wifi-off', 'battery': 'battery-full', 'system-search': 'search', 'preferences-system-notifications': 'bell', 'media-playback-start': 'play', 'media-playback-pause': 'pause',
  'window-minimize': 'minus', 'window-maximize': 'square', 'window-close': 'x', 'system-software-install': 'package-plus', 'user-identity': 'circle-user', 'system-lock-screen': 'lock'}
icons = collections.OrderedDict()
CAT_MINE = {'undo': 'texte', 'redo': 'texte'}
for name, svg in mine.items(): icons[name] = (elems(svg), 'baura')
# ---- 2. sélection Lucide pour un OS -----
BAN = set(json.load(open(luc.parent / 'brand-stopwords.json')).keys())
core = """folder folder-open folder-plus folder-minus folder-search folder-lock folder-archive folder-down folder-up folder-sync folder-tree folder-input folder-output folder-check folder-x
file file-text file-plus file-minus file-check file-x file-search file-lock file-archive file-audio file-video file-image file-code file-spreadsheet file-pen file-down file-up file-chart-column file-cog file-warning file-question
hard-drive hard-drive-download hard-drive-upload usb database server server-cog cpu memory-stick microchip monitor monitor-smartphone monitor-speaker laptop smartphone tablet tv printer scanner mouse keyboard gamepad-2 headphones speaker mic camera webcam battery battery-charging battery-full battery-low battery-medium battery-warning plug plug-zap power power-off
wifi wifi-off wifi-high wifi-low wifi-zero bluetooth bluetooth-connected bluetooth-off signal signal-high signal-low signal-zero globe globe-lock network cable ethernet-port router radio antenna cast airplay rss unplug
settings settings-2 sliders-horizontal sliders-vertical wrench hammer cog toggle-left toggle-right bolt user user-plus user-minus user-check user-x users user-round user-cog circle-user contact id-card key key-round lock lock-open shield shield-check shield-alert shield-off shield-x fingerprint scan-face scan-eye eye eye-off bug
home house building building-2 store map map-pin navigation compass locate crosshair route milestone
bell bell-off bell-ring bell-plus message-circle message-square messages-square mail mail-open mail-plus mail-check mail-x inbox send phone phone-call phone-off video video-off at-sign megaphone
clock clock-3 alarm-clock timer timer-off hourglass calendar calendar-days calendar-plus calendar-check calendar-clock calendar-x history watch sun moon sunrise sunset cloud cloud-sun cloud-rain cloud-snow cloud-lightning cloud-download cloud-upload cloud-off thermometer wind droplet
search zoom-in zoom-out filter funnel list list-checks list-filter list-ordered list-plus list-tree list-todo layout-grid layout-list layout-dashboard layout-template layout-panel-left layout-panel-top columns-2 rows-2 panel-left panel-right panel-top panel-bottom sidebar sidebar-close sidebar-open
maximize minimize maximize-2 minimize-2 expand shrink fullscreen move move-diagonal grip grip-vertical grip-horizontal app-window app-window-mac layers layers-2 copy copy-plus clipboard clipboard-check clipboard-copy clipboard-paste clipboard-list scissors pin pin-off pencil pen-line eraser trash-2 recycle archive archive-restore download upload share share-2 external-link link link-2 unlink paperclip save
chevron-left chevron-right chevron-up chevron-down chevrons-left chevrons-right chevrons-up chevrons-down chevrons-up-down arrow-left arrow-right arrow-up arrow-down arrow-up-right arrow-down-left arrow-left-right arrow-up-down arrow-big-up arrow-big-down corner-down-left corner-up-right undo-2 redo-2 rotate-ccw rotate-cw refresh-ccw refresh-cw repeat repeat-1 shuffle skip-back skip-forward step-back step-forward rewind fast-forward play pause square circle-stop circle-play circle-pause volume volume-1 volume-2 volume-x music music-2 disc disc-3 album list-music audio-lines audio-waveform film clapperboard image images image-plus camera-off aperture
type heading-1 heading-2 heading-3 pilcrow quote text-cursor text-cursor-input case-sensitive baseline highlighter palette paintbrush paint-bucket pipette brush ruler crop frame vector-square pen-tool spline shapes square-dashed circle-dashed wand-sparkles sparkles sparkle wand
code code-xml terminal square-terminal braces brackets git-branch git-commit-horizontal git-merge git-pull-request package package-open package-plus package-check package-x boxes blocks puzzle component cuboid box
bar-chart-3 chart-bar chart-line chart-pie chart-column chart-area chart-scatter activity gauge trending-up trending-down presentation table table-2 sigma calculator percent hash
check check-check circle-check circle-x circle-alert circle-help circle-plus circle-minus triangle-alert octagon-alert info x plus minus ban badge-check badge-alert flag bookmark star heart thumbs-up thumbs-down smile frown meh
circle-question-mark panel-left-close panel-left-open languages accessibility ear ear-off captions subtitles text-quote speech a-large-small zap flame leaf lightbulb lightbulb-off rocket award trophy gift tag tags ticket shopping-cart shopping-bag credit-card wallet banknote coins receipt
hand pointer mouse-pointer mouse-pointer-2 mouse-pointer-click hand-metal move-3d scan scan-line scan-text qr-code barcode nfc usb-c cable-car""".split()
cats = collections.defaultdict(list); meta = {}
for p in sorted(luc.glob('*.svg')):
    n = p.stem; j = json.load(open(p.with_suffix('.json'))); meta[n] = j.get('categories', [])
    if any(b in n for b in BAN) or n in icons: continue
    cats[tuple(meta[n])[:1] or ('autre',)].append(n)
chosen = [n for n in dict.fromkeys(core) if (luc / (n + '.svg')).exists() and n not in icons and not any(b in n for b in BAN)]
FILL = ['devices', 'files', 'layout', 'connectivity', 'security', 'account', 'multimedia', 'time', 'notifications', 'text', 'tools', 'development', 'navigation', 'communication', 'design', 'charts', 'accessibility', 'arrows', 'mail', 'finance', 'home', 'weather', 'cursors', 'science', 'math', 'buildings', 'shapes']
need = 550 - len(icons) - len(chosen); i = 0
while need > 0 and any(cats[(c,)] for c in FILL):
    c = FILL[i % len(FILL)]; i += 1
    while cats[(c,)] and (cats[(c,)][0] in chosen or cats[(c,)][0] in icons): cats[(c,)].pop(0)
    if cats[(c,)]: chosen.append(cats[(c,)].pop(0)); need -= 1
chosen = chosen[:550 - len(icons)]
for n in chosen: icons[n] = (elems((luc / (n + '.svg')).read_text()), 'lucide')
assert len(icons) == 550, len(icons)
bad = [k + '→' + v for k, v in ALIAS.items() if v not in icons]
assert not bad, bad
# ---- 3. sorties -----
names = sorted(icons)
out = ['// GÉNÉRÉ par common/tools/gen_icons550.py — ne pas éditer à la main.',
       '// 97 icônes BAURA + 453 icônes Lucide (ISC, voir LUCIDE-LICENSE.txt). Grille 24×24, trait 1,9, bouts/jointures ronds.',
       '#pragma once', '#include <cstddef>', 'namespace BauraVec::icons {', 'struct Elem { const char* d; bool fill; };', 'struct Icon { const char* name; const Elem* elems; std::size_t n; };']
tab = []
for n in names:
    ident = 'e_' + re.sub(r'\W', '_', n)
    out.append('static const Elem %s[] = {%s};' % (ident, ','.join('{"%s",%s}' % (d, 'true' if fl else 'false') for d, fl in icons[n][0])))
    tab.append('{"%s",%s,%d}' % (n, ident, len(icons[n][0])))
out.append('struct Alias { const char* from; const char* to; };\nstatic const Alias ALIASES[] = {' + ','.join('{"%s","%s"}' % kv for kv in sorted(ALIAS.items())) + '};\nstatic const std::size_t ALIAS_COUNT = %d;' % len(ALIAS))
out.append('static const Icon ALL[] = {\n  ' + ',\n  '.join(tab) + '\n};'); out.append('static const std::size_t COUNT = %d;  // trié par nom (recherche dichotomique possible)' % len(names)); out.append('}')
(here / 'common/icons_data.h').write_text('\n'.join(out) + '\n')
catalog = {n: {'source': icons[n][1], 'categories': meta.get(n, ['baura'])} for n in names}
(here / '09-ICONES/catalogue.json').write_text(json.dumps(catalog, ensure_ascii=False, indent=0))
print(len(names), 'icônes :', sum(1 for n in names if icons[n][1] == 'baura'), 'BAURA +', sum(1 for n in names if icons[n][1] == 'lucide'), 'Lucide')
