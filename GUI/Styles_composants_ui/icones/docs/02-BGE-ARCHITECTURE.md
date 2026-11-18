# BGE — Baura Graphical Engine : comment il assemble et affiche l'interface

BGE est le moteur qui transforme **une description d'interface** (arbre de composants + thème) en **pixels**.
Le code fourni est un moteur complet *de référence* en C++17, sans dépendance, qui compile avec `g++`/CMake :

```
bge/
├─ include/bge/
│  ├─ core.hpp        types (Vec2, Rect, Color), Theme (jetons), CubicBezier, Tween/Animator
│  ├─ gfx.hpp         Canvas, Paint, Path (parseur SVG), Painter (formes, ombre, VERRE, icônes, logo)
│  ├─ scene.hpp       Widget (mesure → placement → dessin → événements), Engine (boucle d'image)
│  ├─ components.hpp  Box, Label, IconW, Button, AppTile, TaskbarPill, WindowW, Toast, TrayPill…
│  └─ icons_data.hpp  97 icônes (GÉNÉRÉ depuis baura-icons.js)
├─ examples/desktop_demo.cpp   assemble un bureau complet → desktop_light.ppm / desktop_dark.ppm
├─ src/platform_sdl2.cpp       fenêtre + entrées (SDL2) — écrit, non testé ici
├─ tools/gen_icons.py          baura-icons.js → icons_data.hpp
├─ ports/bge_core.rs           cœur en Rust (jetons, courbes, logo) — testé
└─ ports/bge_tokens.py         jetons + logo en Python
```
Compiler et lancer : `cd bge && g++ -std=c++17 -O2 -Iinclude examples/desktop_demo.cpp -o demo && ./demo` (ou `cmake -B build && cmake --build build`).

## 1. Vue d'ensemble : le pipeline d'une image

```
 entrées ──► Engine::dispatch ──► Widget::onEvent (survol, clic)
                │
 horloge ──► Animator::update(dt)   (modifie scale, opacity, couleurs… des widgets)
                │
        ┌───────▼────────┐   ┌──────────────┐   ┌─────────────────────────┐   ┌──────────┐
        │ 1. MESURE      │──►│ 2. PLACEMENT │──►│ 3. DESSIN (ordre Z)     │──►│ 4. PRÉSENT│
        │ Widget::measure│   │ Widget::layout│   │ fond → Widget::paint…   │   │ Canvas→  │
        │ taille voulue  │   │ cadre (Rect)  │   │ (Painter + thème)       │   │ écran    │
        └────────────────┘   └──────────────┘   └─────────────────────────┘   └──────────┘
```
`Engine::frame(dt, wallpaper)` exécute exactement ces étapes : `anim.update` → `root.layout` → effacement + fond d'écran → `root.paint`.

## 2. Assembler : l'arbre de composants

Une interface est un **arbre** de `Widget`. Rien n'est dessiné « en dur » : on compose.

```cpp
Engine e(1440, 900, texte);
auto* dock = e.root.add<TaskbarPill>();                 // pilule verre 52 px
dock->style.absolute = true; dock->style.x = 10; dock->style.y = 900-10-52;
dock->add<StartButton>();                               // symbole BAURA
dock->add<DockApp>(tile(0x60A5FA,0x2563EB), "pen", /*ouverte*/true);
```
* `add<T>(…)` crée l'enfant et le rattache (propriétaire unique : `unique_ptr`).
* Les **composants BAURA** (`WindowW`, `Toast`…) sont des widgets qui redessinent la recette du §3 du doc de design ; les **conteneurs** (`Widget`, `Box`) ne font que placer.
* L'ordre des enfants = ordre de dessin (le dernier est au‑dessus) et ordre inverse pour la détection de clic.

## 3. Mise en page (flex simplifié)

`Style` reprend les notions CSS utiles : `dir` (Row/Col), `align` (Start/Center/End/Stretch), `justify` (Start/Center/End/Between), `gap`, `pad`, `w/h` (fixes), `grow`, `absolute + x/y`.

1. **`measure`** : une feuille renvoie sa taille intrinsèque (une icône = sa taille, un `Label` = largeur du texte via `TextProvider`) ; un conteneur additionne ses enfants + gaps + padding.
2. **`layout(box)`** : le conteneur reçoit un rectangle, retire son padding, répartit l'axe principal (tailles fixes, puis l'espace restant aux enfants `grow`), aligne l'axe transversal, puis appelle `layout` sur chaque enfant. Les enfants `absolute` sont placés à `(x,y)` du parent et sortent du flux (barre des tâches, notifications).
3. Le résultat est `frame` (un `Rect` en pixels écran) stocké dans chaque widget.

Le CSS du prototype (`display:flex`, `gap`, `padding`, `position:fixed`) se traduit donc 1‑pour‑1.

## 4. Dessiner : primitives et pile de transformations

Le **`Painter`** est la seule porte vers les pixels. Primitives : `fillRRect`, `strokeRRect`, `shadow`, `glass`, `fillPath`, `strokePath`, `icon`, `logo`.

* **Rectangle arrondi** : pour chaque pixel on évalue la *distance signée* au rectangle arrondi ; la couverture `clamp(0.5 − d)` donne un bord anti‑crénelé parfait, quel que soit le rayon. Une bordure = couverture(extérieur) − couverture(intérieur).
* **Dégradés** (`Paint::linear(angle, c0, c1)`) : `t` = projection du pixel sur la direction CSS (`0° = vers le haut`), normalisée par la diagonale projetée de la boîte.
* **Chemins** : `Path::fromSVG` lit l'attribut `d` (M L H V C S Q T A Z, absolu/relatif), aplatit courbes et arcs (conversion « endpoint → centre » de la norme SVG) en polylignes.
  * *Remplissage* : balayage par lignes avec 4 sous‑lignes, règle non‑zéro, couverture horizontale exacte.
  * *Trait* : distance du pixel au polyligne ; `couverture = clamp(w/2 − d + .5)` ⇒ bouts et jointures **ronds** sans code spécial, et pas de double opacité aux croisements.
* **Pile** (`push/pop`, `translate`, `scaleAbout`, `opacity`, `clip`) : un widget qui s'anime (`scale`, `opacity`) pousse une transformation ; **tout** son sous‑arbre en hérite. C'est ainsi que la fenêtre « pop » à l'ouverture sans buffer intermédiaire.
* **Ombre** : couverture de la forme décalée → flou → multipliée par *(1 − couverture de la forme)* (découpée hors forme comme `box-shadow`).

### Le verre, étape par étape (`Painter::glass`)
1. Le moteur a **déjà peint** le fond d'écran et tout ce qui est sous le widget (ordre Z).
2. On copie la zone, on la floute (3 passes de *box blur* ≈ gaussienne, σ = blur/2), puis on applique la matrice de saturation ×1,9.
3. On **remplace** les pixels sous la forme (masque arrondi) par cette version floutée.
4. On peint la teinte (`glass`), la bordure 1 px et le reflet haut.
C'est pourquoi **l'ordre de dessin est une contrainte forte** : un verre ne floute que ce qui est dessiné avant lui. La barre des tâches et la fenêtre sont donc après le fond, et un toast après la fenêtre.

## 5. Thème : une seule source de vérité

`Theme` contient tous les jetons. `Engine::th = Theme::darkTheme()` suffit à basculer ; chaque composant lit `c.th.*` au moment du dessin (jamais de couleur en dur, sauf tuiles d'application). Accent, contraste, mouvement réduit = autres champs/drapeaux de `Theme`, équivalents des attributs `data-*` du prototype.

## 6. Mouvement

`Tween{from,to,dur,ease,apply}` modifie une valeur chaque image ; `Animator` les exécute et les retire à la fin. Les courbes `SPRING` (.3, 1.45, .45, 1) et `EASE_OUT` sont résolues comme en CSS (Newton + bissection). Exemple (ouverture de fenêtre, dans la démo) :

```cpp
win->scale = .94f; win->opacity = 0;
Tween t; t.dur = .3f; t.ease = SPRING;
t.apply = [win](float v){ win->scale = .94f + .06f*v; win->opacity = std::min(1.f, v*3); };
engine.anim.add(t);
```
Le survol d'un `Button` crée automatiquement un fondu 150 ms vers `hover`.

## 7. Texte
Le moteur n'impose pas de police : l'interface `TextProvider { measure, draw }` isole tout le texte. Pour la production :
* **FreeType** (rastérisation de glyphes Inter) + **HarfBuzz** (mise en forme, ligatures, arabe/RTL pour la langue AR) ; atlas de glyphes en cache ; `measure` renvoie largeur/hauteur de ligne.
* Windows : DirectWrite ; macOS : CoreText ; Linux : Pango.
La démo utilise `PlaceholderText` (barres proportionnelles) pour rester sans dépendance — c'est un choix de démonstration, pas un rendu final.

## 8. Entrées et événements
`Engine::dispatch(Event)` : (1) **hit‑test** — parcours en profondeur dans l'ordre inverse du dessin, premier widget `interactive()` dont le `frame` contient le point ; (2) suivi `hover` (entrée/sortie) ; (3) capture au `Down` pour que le `Up` revienne au même widget ; (4) **bulle** : `onEvent` du widget, puis de ses parents, jusqu'à ce que l'un renvoie `true`. Un `Button` déclenche `onClick` sur `Up` dans son cadre.

## 9. Du prototype au système réel : où BGE se branche

```
 Application (Skrib, Numbro…)  ──►  arbre de Widgets
            │                              │
      Gestionnaire de fenêtres  ──►  Engine (compositeur)  ──►  Plateforme (IPlatform)
   (z‑order, snap, onglets, UAC)        │                          ├─ SDL2 / Wayland / DRM‑KMS (Linux)
                                        └─ Painter                 ├─ Win32 / DXGI
                                           ├─ Logiciel (ce dépôt)  └─ Cocoa / Metal
                                           ├─ Skia (CPU/GPU)
                                           └─ Vulkan/OpenGL
```
* **Plateforme** : crée la fenêtre/l'écran, lit souris/clavier/tactile, appelle `Engine::frame` à chaque vsync, envoie la `Canvas` à l'écran (`src/platform_sdl2.cpp`).
* **Moteur de rendu** : `Painter` est l'unique point à réimplémenter pour changer de technologie. Correspondance GPU :
  | Opération | GPU |
  |---|---|
  | `fillRRect` / bord | shader SDF d'un quad (même formule) |
  | `shadow` | shader SDF flouté ou ombre pré‑rendue en texture 9‑slice |
  | `glass` | rendu de la scène dans une texture, passes de flou séparables (Kawase/Dual) sur le tiers de la résolution, saturation dans le shader final |
  | `fillPath` / icônes | atlas d'icônes rastérisées (SDF/MSDF) ou tessellation ; les icônes sont des chemins simples |
* **Gestionnaire de fenêtres** : maintient la liste des `WindowW` (ordre Z, focus, snap, onglets, effet « moving » = `scale .97` + `opacity .85` pendant le glisser, badge administrateur) ; chaque fenêtre est un sous‑arbre dont le contenu est fourni par l'application.
* **Applications** : un contrat équivalent à `AppViews.<id>(root, ctx)` du prototype → `std::unique_ptr<Widget> createView(AppContext&)` + `open(args)` + `destroy()`.

## 10. Performances (à faire pour la production)
* Le flou est l'opération coûteuse : le mettre en **cache** tant que ce qui est derrière ne change pas (fond statique), et le faire à **¼ de résolution** puis agrandir.
* **Rectangles sales** : ne repeindre que les widgets modifiés (animation, survol) et leurs superpositions.
* Pré‑rastériser les icônes par taille (atlas) ; ne ré‑aplatir les `Path` qu'au changement d'échelle.
* Mesuré ici : 1440×900 avec verres et ombres ≈ 0,12 s/image en logiciel (≈ 8 i/s) — **pas encore temps réel (60 i/s)** ; c'est une référence de comportement, pas le moteur final.

## 11. Ajouter un composant (recette)
1. Lire sa fiche dans `01-DESIGN-SPEC.md` (dimensions, jetons).
2. Dériver de `Widget` (ou `Box`), fixer `style.w/h`, `pad`, `gap`.
3. `paintSelf` : appeler dans l'ordre ombre → verre/fond → bordure → icônes → texte, avec les jetons `c.th`.
4. Si interactif : `interactive()` → `true` et `onEvent`.
5. L'ajouter à l'arbre avec `add<T>()`.

## 12. Équivalence prototype Web → BGE
| Web (prototype) | BGE |
|---|---|
| `:root{--glass,…}` / `data-theme` | `Theme` / `Theme::darkTheme()` |
| `backdrop-filter:blur() saturate()` | `Painter::glass` |
| `box-shadow` | `Painter::shadow` |
| `border-radius` | rayon du SDF |
| `linear-gradient(145deg,…)` | `Paint::linear(145,…)` |
| `display:flex; gap; padding` | `Style{dir,gap,pad,justify,align}` |
| `position:fixed` | `Style.absolute` |
| `transition` / `cubic-bezier` | `Tween` + `CubicBezier` |
| `<svg>` (logo, `BI.i()`) | `Path::fromSVG`, `Painter::logo/icon` |
| `postMessage` entre calques | file d'événements du compositeur |
| `Baura.store` (localStorage) | service de configuration (fichier/registre) |
