# 00 — Audit du GUI actuel du dépôt

Base : commit `86642c0` (« Add seL4 source check and GUI preview »). Méthode : lecture des sources `GUI/` + `make gui-preview`
(rendu : `render/gui-actuel-du-depot.png`, 800×600).

## Ce qui existe et fonctionne (hôte)
| Brique | Dossier | Constat |
|---|---|---|
| Canevas ARGB8888, clip, rect arrondi, dégradé vertical, ligne | `GUI/BGE/renderer.*` | Solide, borné ; rect arrondi **non anti-crénelé** |
| Flou + panneau de verre | `GUI/BGE/effects_engine.*` | Flou « boîte » à 1 passe, rayon ≤ 32, **sans saturation, sans ombre, sans reflet** |
| Fenêtres, focus, glisser, réduire/agrandir | `GUI/BGE/window_manager.*`, `GUI/WINDOW_MANAGER/` | OK, **sans animation** |
| Serveur d'affichage, compositeur | `GUI/DISPLAY_SERVER/`, `GUI/COMPOSITOR/` | Surfaces bornées + z-order |
| Bureau, connexion, installateur | `GUI/DESKTOP/`, `GUI/LOGIN_SCREEN/`, `BOOT/INSTALLER/` | Squelettes pilotés par callbacks |
| Entrée PS/2, plateforme framebuffer | `GUI/INPUT/`, `GUI/PLATFORM/` | Décodeur logiciel ; pas encore branché à seL4 |

## Écarts avec l'identité BAURA
| # | Écart | Preuve dans le code | Document |
|---|---|---|---|
| 1 | **Palette** bleu/cyan/violet-gris sombre ≠ charte indigo `#4F46E5` / violet `#8B5CF6` / lavande `#C4B5FD` / or `#AB8E18`; pas de thème clair | `GUI/UI_FRAMEWORK/baura_theme.h` | 01 |
| 2 | **Marque** = lettre « B » pixelisée 5×7 dans un carré bleu, pas le symbole (barre + 2 demi-disques + point or) | `baura_brand.h::draw_mark` | 01 |
| 3 | **Icônes** : formes tracées à la main (≈ 24 noms) | `renderer.cpp::draw_icon`, `papirus_icon_catalog.cpp` | 01 |
| 4 | **Texte** : police bitmap 5×7 majuscules, sans accents ni minuscules | `renderer.cpp::glyph_row` | 02 |
| 5 | **Verre** non fidèle (voir rendu 03) | `effects_engine.cpp` | 03 |
| 6 | **Bords crénelés** (rect arrondi, cercles) | `renderer.cpp::draw_rounded_rect` | 01 |
| 7 | Pas de **composants** : champ de saisie, liste défilante, onglets, interrupteur, curseur, menu | absents de `ui_framework` | 04 |
| 8 | Pas d'**animation** (ressort, fondu) | `window_manager.cpp` | 05 |
| 9 | Marque/splash **jamais affichés au vrai boot** | roadmap §2 | 06 |
| 10 | Pas de **RTL/arabe**, pas d'accessibilité | — | 08 |

## Ordre recommandé
01 → 02 → 03 (identité visible partout), puis 04 → 05 (interaction), puis 06 → 07 (boot réel + bureau assemblé).

## Inventaire écran par écran et cible
Rendus actuels : `render/actuel_planche.png` (12 écrans capturés depuis le dépôt par `code/capture_current.cpp`). Pour **chaque écran**, la version cible, le fichier du dépôt à modifier et la phase de migration sont dans `../07-BUREAU-CIBLE/README.md` §1 et `../10-FEUILLE-DE-ROUTE/README.md` §3.

| Écran actuel | Défauts principaux relevés | Cible |
|---|---|---|
| Bureau | barre pleine largeur collée, texte « SEARCH »/« FICHIERS » en majuscules, aucune hiérarchie, fenêtre vide | `07` : `bureau_clair/sombre` |
| Démarrer | liste 2×2 minuscule, pas de récents ni de compte | `07` : `demarrer_*` |
| Recherche | champ + zone vide | `07` : `recherche_sombre` |
| Notifications | cadre sans structure | `07` : `centre-actions_sombre` |
| Connexion | carte sombre générique, message d'erreur en majuscules | `07` : `connexion_sombre` |
| Installateur (6 étapes) | même carte, aucun repère de progression | `07` : `installateur_1…4` |
| Bulle système | libellé tronqué, courbe en dents de scie | `07` : `bulle-et-menu_sombre` |
