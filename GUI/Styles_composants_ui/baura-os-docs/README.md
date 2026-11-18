# BAURA OS — Documents de développement du GUI (code + rendu côte à côte)

Dossier de travail **à côté** du dépôt `Bryant273/BAURA-OS` : rien n'y est modifié ni poussé. Chaque sujet manquant a son dossier avec
`README.md` (explication), `code/` (C++ qui **compile contre le BGE existant**), `render/` (image produite par ce code).

> Le prototype web (HTML/CSS) n'est **pas** l'interface finale : c'est une inspiration. Ces documents portent la même *identité* (charte, verre, icônes, texte) dans le vrai moteur C++ du dépôt.

| # | Sujet | État | Rendu / entrée |
|---|---|---|---|
| 00 | Audit du GUI actuel + manques | ✅ | `00-AUDIT-GUI/render/actuel_planche.png` |
| 01 | Identité & vectoriel : charte, symbole réel, AA | ✅ code + rendu | `01-…/render/identite-avant-apres.png` |
| 02 | Texte réel (Inter cuit, accents, crénage) | ✅ code + rendu | `02-…/render/texte-avant-apres.png` |
| 03 | Verre fidèle + ombres | ✅ code + rendu | `03-…/render/verre-avant-apres.png` |
| 04 | Composants (champs, listes, onglets, interrupteurs, menus) | ✅ code + rendu + tests | `04-…/render/composants_*.png` |
| 05 | Mouvement branché sur `BGEWindowManager` (+ patch de 2 getters) | ✅ code + rendu + tests | `05-…/render/*.png` |
| 06 | Démarrage : splash animé, framebuffer Multiboot2/UEFI/seL4 | ✅ code + rendu + tests (seL4 : conception) | `06-…/render/splash-sequence.png` |
| 07 | **Bureau cible (« glow up »)** : 15 écrans de la coque + 12 applications | ✅ code + 27 rendus | `07-…/render/ecrans_planche.png`, `apps_planche.png` |
| 08 | Arabe / RTL, accessibilité (contraste, clavier, contraste élevé) | ✅ code + rendu + tests | `08-…/render/` |
| 09 | **550 icônes** (97 BAURA + 453 Lucide) + catalogue + alias | ✅ | `09-…/CATALOGUE.md`, `render/icones_page*.png` |
| 10 | **Feuille de route** : de l'état du dépôt à la cible (phases P0–P15) | ✅ | `10-FEUILLE-DE-ROUTE/README.md` |
| 11 | **Icônes d'applications BAURA** (système + applications existantes, cohérentes ; inspirées du style Papirus, dessins originaux) | ✅ code + 122 icônes + SVG | `11-…/render/appicons_clair.png` |
| 12 | **Backend Rust + ABI C + enveloppe C++** (prefs, moniteur, icônes, shell, i18n) | ✅ 34 tests Rust + test C++ | `12-BACKEND/README.md` |
| 13 | **Le Moniteur** (gestionnaire des tâches simple, alimenté par le backend) | ✅ code + 7 rendus | `13-MONITEUR/render/` |
| 14 | **Mode natif** : faire « OS » plutôt qu'« appli web » | ✅ code + rendus (Fichiers) | `14-NATIF/render/natif_avant-apres.png` |

**Par où commencer :** `10-FEUILLE-DE-ROUTE/README.md` (le plan), puis `07-BUREAU-CIBLE/` (la cible visuelle ; **bureau par défaut = barre coupée en deux**, autres dispositions = options), `13-MONITEUR/` et `14-NATIF/`.

## Reconstruire tout
```
BAURA_OS_REPO=/chemin/vers/BAURA-OS ./build_all.sh      # tout ; ou ./build_all.sh 06 08 pour certains documents ; STRICT=1 = -Werror comme le Makefile du dépôt
# nécessite g++ (C++17), python3 + Pillow (Raqm), polices Inter et DejaVu, et cargo (docs 12–13) ; ne modifie pas le dépôt (le patch du 05 s'applique à une copie)
```
`common/` contient le code partagé (copiable dans `GUI/UI_V2/` quand tu le décides) : `baura_vector.h`, `baura_text.h`, `baura_glass.h`, `baura_theme_brand.h`, `baura_ui.h`, `baura_textedit.h`, `baura_motion.h`, `baura_wm_motion.h`, `icons_data.h` (550 icônes), et les polices cuites `inter.bgf`, `mono.bgf`, `arabic.bgf`.

## Limites connues (honnêtes)
* Le code des documents 01–03 utilise la STL (`vector`, `sort`, `cmath`) : parfait pour une tâche utilisateur seL4 avec libc++, **pas** pour du code noyau/freestanding. Le style « borné, sans exception » du dépôt reste à appliquer (allocations de tampons de flou).
* **Performance : un bureau complet 1280×720 coûte ≈ 186 ms (≈ 5 i/s) sans cache** (mesuré, voir `10-FEUILLE-DE-ROUTE` §4). Ce code est une référence visuelle ; le cache, les rectangles sales et le SIMD sont prévus en P11 (et un premier lot dès P3/P6).
* Aucun de ces documents n'a tourné sur le vrai boot seL4/QEMU : ils sont validés sur l'hôte, comme `make test-gui`. Le mappage du framebuffer sous seL4 (06 §3.3) est de la **conception non compilée**.
* Les 12 applications et les écrans de 07 sont des **maquettes d'un état** (pas d'interaction) ; arabe : jonction/bidi simplifiés (pas d'UAX #9 complet) et police de démonstration.
* Estimations d'effort de la feuille de route : ±50 %, à recalibrer après la phase P1.
