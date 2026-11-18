# BAURA OS — Spécification visuelle et graphique

Document de référence pour **reproduire l'interface de BAURA OS en C++** (ou tout autre langage) avec le BGE.
Chaque valeur ci‑dessous est tirée du prototype HTML/CSS (`baura-theme.css`, `os-style.css`, `baura-icons.js`) ;
le code correspondant est dans `bge/include/bge/` (`core.hpp` = jetons, `gfx.hpp` = rendu, `components.hpp` = composants).

> **Aperçu produit par le BGE (C++)** : `docs/bge-demo-light.png` et `docs/bge-demo-dark.png` — générés par `bge/examples/desktop_demo.cpp`.
> Le texte y apparaît sous forme de barres (voir §10 : la police reste à brancher).

---

## 1. Principes

| Principe | Traduction |
|---|---|
| Simple comme Windows, élégant comme macOS | Barre des tâches basse, menu démarrer, fenêtres à onglets ; verre, arrondis, ombres douces |
| Un seul langage de forme | Tout est **rectangle arrondi**, **demi‑disque** ou **trait arrondi** (le symbole) |
| Une seule famille d'icônes | Vectorielles, grille 24×24, trait 1,9, extrémités et jointures rondes, `currentColor` — **plus aucun emoji** |
| Thème = jetons | Clair/sombre ne changent que les valeurs des jetons (§2), jamais la structure |

## 2. Jetons de couleur

### 2.1 Marque
| Jeton | Clair | Sombre | Rôle |
|---|---|---|---|
| `b1` indigo | `#4F46E5` | `#6D66F2` | barre du symbole, accent principal |
| `b2` violet | `#8B5CF6` | `#9D77FF` | demi‑disque haut, dégradés |
| `b3` lavande | `#C4B5FD` | `#CFC3FF` | demi‑disque bas, surfaces douces |
| `b4` or | `#AB8E18` | `#D0AE2A` | point doré, indicateur « app ouverte » |
| `nuit` | `#0F0B2E` | — | fonds très sombres |

### 2.2 Interface (RGBA)
| Jeton | Clair | Sombre |
|---|---|---|
| `fg` texte | `#17133d` | `#f4f2ff` |
| `mut` texte discret | `#5f5a88` | `#b4b0da` |
| `glass` | rgba(255,255,255,.34) | rgba(40,32,104,.36) |
| `glass-strong` | rgba(255,255,255,.55) | rgba(28,22,74,.62) |
| `glass-line` (bordure) | rgba(255,255,255,.70) | rgba(255,255,255,.20) |
| `hair` (filets) | rgba(79,70,229,.14) | rgba(255,255,255,.12) |
| `hover` | rgba(79,70,229,.10) | rgba(255,255,255,.10) |
| `press` | rgba(79,70,229,.18) | rgba(255,255,255,.17) |
| `field` (champs) | rgba(255,255,255,.42) | rgba(255,255,255,.10) |
| `win` (corps de fenêtre) | rgba(255,255,255,.74) | rgba(26,21,68,.80) |
| `win-bar` | rgba(255,255,255,.42) | rgba(34,28,86,.50) |
| `accent` | `#4F46E5` | violet mélangé 62 % avec blanc |
| `err` / `ok` | `#d6303c` / `#1f8a57` | `#ff7a84` / `#5fd49b` |

Constantes de verre : `blur = 36 px`, `saturate = 1,9`. Mode « transparence réduite » : `blur = 0`, `sat = 1`, surfaces **opaques** (`#faf9ff` clair, `#1c1750` sombre).
→ C++ : `bge::Theme::light()` / `Theme::darkTheme()` (`core.hpp`).

## 3. Le symbole (logo)

Boîte de dessin `viewBox = 150 29 199 420` (rapport 199:420).

| Élément | Géométrie | Couleur |
|---|---|---|
| Barre | rect `x150 y29 w48 h420 rx24` (pilule verticale) | `b1` |
| Demi‑disque haut | `M248 29 A101 102.5 0 0 1 248 234 Z` (ouvert vers la gauche, collé à la barre) | `b2` |
| Demi‑disque bas | `M248 245 A101 102 0 0 1 248 449 Z` | `b3` |
| Point | rect `x316 y221 w33 h36 rx15` | `b4` |

Zone de protection ≥ ½ largeur de la barre. Hauteur minimale 16 px. Sur fond sombre, utiliser les variantes sombres de `b1..b4`.
→ C++ : `Painter::logo(rect, b1, b2, b3, b4)`.

## 4. Jeu d'icônes (97 icônes)

* Grille 24×24, trait **1,9**, `stroke-linecap/linejoin: round`, `fill: none` (sauf points/étoiles pleines : `fill=currentColor`).
* Taille d'usage : 17 px (barres d'outils), 15 px (boutons d'état), 18–22 px (listes), 28 px (cartes de l'installateur).
* Couleur = couleur du texte courant (`currentColor`) ; blanc sur tuile d'application.
* Source unique : `baura-icons.js` ; le script `bge/tools/gen_icons.py` les convertit en tableau C++ (`icons_data.hpp`, rect/cercles convertis en chemins). Toute nouvelle icône ajoutée au JS est reprise par une régénération.
* Familles : édition (undo, redo, bold, italic, underline, alignL/C/R, listUl/Ol, table, image, link), fichiers (copy, trash, lock, eye), média (play, pause, volume, mute, repeat, pip, fullscreen, film), système (cpu, memory, monitor, hdd, keyboard, globe, power, zap, shield), navigation (chev*, home, refresh, bookmark, menu, search), courrier (mail, mailOpen, inbox, send, reply, forward, pen), état (check, warn, info, pin, star, starFill).
* Drapeaux de langue : **pastilles** `FR EN ES DE PT AR` (dégradé `b2→b1`, texte blanc 11 px gras, rayon 8 px, hauteur 26 px) — pas d'emoji‑drapeaux.

→ C++ : `Painter::icon("undo", rect, couleur, 1.9f)`.

## 5. Typographie

* Famille : **Inter** (repli : Segoe UI, system‑ui, sans‑serif). Terminal : **JetBrains Mono** (repli SF Mono/Menlo/Consolas).
* Échelle : 11 (kbd, pastilles) · 12,5 (onglets, métadonnées) · 13 (corps compact) · 14 (corps, titres de fenêtre en 600) · 15 (titres d'options) · 20 (titres de lecteur) · 26 (titres de page) · 40+ (titres d'accueil, interligne 1,05, `letter-spacing −.01em`).
* Graisses 400 / 500 / 600 / 700. Lissage niveaux de gris.

## 6. Formes, espacements, ombres

| Élément | Rayon | Notes |
|---|---|---|
| Boutons d'outil | 11–12 px | zone 34–36 × 32–34 |
| Tuiles d'application | 13–18 px | dégradé 145°, bord blanc 35 %, ombre `0 4 10` |
| Pilule barre des tâches | 20 px | hauteur 52 |
| Pilule d'état (haut‑droite) | 16 px | hauteur 40 |
| Notification | 20 px | largeur 332 |
| Fenêtre | 22–24 px | barre de titre 46 px |
| Boîtes de dialogue (UAC, assistants) | 24–26 px | largeur 440 |
| Cartes de l'installateur | 28 px | |

Grille d'espacement : 2 · 4 · 6 · 8 · 10 · 12 · 14 · 18 · 24 px.
**Ombre standard** (`--shadow`) : `0 12 34 rgba(52,40,140,.22)` + `0 2 6 rgba(52,40,140,.10)` + reflet interne haut `inset 0 1 0 rgba(255,255,255,.75)` (sombre : `0 14 38 rgba(0,0,0,.5)` + reflet .20). Fenêtre : ombre supplémentaire `0 30 70 rgba(15,11,46,.35)`.

## 7. Le verre (glassmorphism)

Recette d'une surface verre, **dans cet ordre** :
1. **Ombre portée** découpée hors de la forme.
2. **Flou d'arrière‑plan** : copier ce qui est déjà dessiné sous la forme, flou gaussien σ = blur/2 (= 18 px).
3. **Saturation** ×1,9 (matrice CSS `saturate()`).
4. **Teinte** : remplir avec `glass` / `glass-strong` / `win`.
5. **Bordure** 1 px `glass-line` (trait intérieur).
6. **Reflet haut** : liseré blanc dégradé de 18 px (alpha .75 → 0 ; .20 en sombre).

→ C++ : `Painter::glass(...)` (`gfx.hpp`) ; l'ordre de dessin du moteur garantit que « ce qui est derrière » est déjà peint.

## 8. Composants

| Composant | Anatomie | Classe C++ |
|---|---|---|
| **Barre des tâches** | pilule verre 52 px en bas‑gauche (10 px des bords), padding 5 : bouton démarrer (symbole 18×26 dans 44×42) · séparateur 1×24 · applications 42×42 (tuile 38, rayon 13) · indicateur « ouverte » = barre or 14×3 sous la tuile | `TaskbarPill`, `StartButton`, `DockApp` |
| **Pilule d'état** | haut‑droite, boutons 34×32 rayon 12 (volume, activité, énergie, notifications), pastille rouge 16 px avec compteur | `TrayPill` |
| **Fenêtre** | verre `win`, barre 46 px (`win-bar` + filet `hair`), icône d'app 22 px rayon 7, titre 14/600, boutons 32×24 rayon 9 : réduire/agrandir (`b3` 35 %), fermer (`b4` 22 %), icônes 14 px trait 2,2 ; onglets à la Windows 11 (rayon 11 11 0 0, hauteur de barre, max 190 px) | `WindowW` |
| **Notification** | 332 px, verre fort, padding 12×14, tuile 40 rayon 13, titre 14/600, texte 12,5 `mut` | `Toast` |
| **Bouton** | transparent ; survol `hover`, appui `press` (fondu 150 ms) | `Button` |
| **Menu contextuel / popovers** | verre fort, rayon 14–20, éléments hauteur ~34, `kbd` 11/600 `mut` | `Box{glass}` |
| **Champs** | fond `field`, bord `field-line`, rayon 12 | `Box` |
| **Interrupteur** | piste 44×26 ; pastille blanche 20 px (ressort) | à composer |
| **UAC / assistants** | boîte 440 px rayon 24–26, verre fort, voile sombre derrière, icône bouclier | `Box` |
| **Terminal** | fond rgba(10,7,32,.86), texte `#e8e4ff`, mono 13 px, invite `C:/user@baura /chemin ❯` (❯ or si admin) | `Widget` + texte |
| **Installateur** | carte 28 px à 2 colonnes : rail gauche (étapes, matériel détecté) + contenu à défilement interne, barre de progression 10 segments (dégradé `b1→b2`) | `Box`/`Widget` |

Applications (tuile, dégradé 145°) : Fichiers `6D66F2→4F46E5` · Notes/Skrib bleu `60A5FA→2563EB` · Numbro vert `34D399→047857` · Data violet `A78BFA→6D28D9` · WePoint orange `FB923C→EA580C` · Bmail turquoise · Corbeille gris `9aa0c8→6b7199` · alerte rose `FB7185→E11D48`.

## 9. Mouvement

| Usage | Durée | Courbe |
|---|---|---|
| Ouverture fenêtre / dialogue | 300 ms | `--spring` = cubic‑bezier(.3, 1.45, .45, 1), échelle .94 → 1 + fondu |
| Menus / panneaux | 300 ms | spring, `translateY(−10px) scale(.97)` → 0 |
| Survol / appui | 150 ms | ease |
| Entrées générales | 250–400 ms | `--ease-out` = cubic‑bezier(.16, 1, .3, 1) |
| Déplacement d'une fenêtre | — | la fenêtre rétrécit légèrement et devient translucide tant qu'on la tient |

→ C++ : `CubicBezier SPRING/EASE_OUT`, `Tween`, `Animator` (`core.hpp`). Mode « réduire les animations » : durées à 0.

## 10. Ce que la démo C++ ne fait pas encore
* **Texte réel** : `PlaceholderText` dessine des barres. En production, implémenter `TextProvider` avec FreeType + HarfBuzz (voir `02-BGE-ARCHITECTURE.md` §7).
* Clip arrondi des enfants, rotation, ombres internes complètes, cache de flou (voir §9 de l'architecture : optimisation GPU).
