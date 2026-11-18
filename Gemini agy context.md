# Contexte Global du Projet — BAURA OS

> **Document de référence pour le contexte architectural et technique de BAURA OS**  
> **Maintenu par :** Gemini (Antigravity) en synchronisation continue avec `Gemini agy actions.md`  
> **Dernière mise à jour :** 16 novembre 2026 (Intégration du véritable rendu natif BGE haute fidélité)

---

## 1. Identité et Vision du Projet

**BAURA OS** est un système d'exploitation modulaire et sécurisé conçu initialement pour les architectures **PC x86_64**. 

Le système repose sur une séparation rigoureuse en couches :
1. **Couche Micro-noyau (Mécanismes bas niveau) — [seL4](file:///c:/Users/SDA/Downloads/BAURA-OS/KERNEL/L4/sel4) :**
   * Version de référence : seL4 `16.0.0-dev`.
   * Rôle exclusif : gestion des primitives fondamentales d'isolation et d'exécution — threads/tâches, espaces d'adressage paginés, communication inter-processus (IPC), interruptions matérielles, et contrôle d'accès basé sur les capacités noyau (**CSpaces**).
   * Vision à long terme : conception future d'un micro-noyau propre nommé `BAURA L4`, tout en maintenant seL4 comme référence certifiée et stable.
2. **Couche de Services & Politiques (OS) — [BAURA Core](file:///c:/Users/SDA/Downloads/BAURA-OS/KERNEL/BAURA_CORE) :**
   * Rôle : applique les politiques du système d'exploitation, supervise les services utilisateurs, fournit le système de fichiers virtuel (VFS), le bus d'événements, la gestion des quotas/ressources et les API publiques du système.
   * BAURA Core ne réimplémente pas les mécanismes bas niveau de seL4 : il interagit avec eux par l'intermédiaire d'adaptateurs de plateforme (`BauraCorePlatform`).
3. **Couche Utilisateur & Interface — [BGE & USERSPACE](file:///c:/Users/SDA/Downloads/BAURA-OS/GUI) :**
   * Rendu logiciel haute fidélité (BGE - BAURA Graphics Engine), compositeur de fenêtres, double barre des tâches translucide en capsules et applications système.

---

## 2. Invariants Architecturaux Fondamentaux

### A. Représentation Canonique des Chemins VFS
Conformément à la spécification formelle ([`VFS_SPEC.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/INFRASTRUCTURE/SPECIFICATIONS/SYSTEM_SPECIFICATIONS/VFS_SPEC.md)) :
* La syntaxe officielle et unique des chemins internes est :
  ```text
  PC/<LECTEUR>:/<CHEMIN>
  ```
  *Exemples :* `PC/C:/Ahmed/Documents`, `PC/D:/Backup/2026/`, `PC/USB:/Photos/`.
* Le préfixe `PC/` abstrait l'arborescence logique des disques et partitions indépendamment du système de fichiers sous-jacent (BAURAFS, ext4, NTFS, FAT32).
* Un moteur de compatibilité ([`translation_engine.cpp`](file:///c:/Users/SDA/Downloads/BAURA-OS/USERSPACE/COMPATIBILITY/TRANSLATION_ENGINE/translation_engine.cpp)) assure la translation automatique des chemins Windows (`C:\...`) et POSIX/Linux (`/home/...`).

### B. Modèle d'Autorité et Sécurité
* Les droits réels proviennent exclusivement des capacités transférées et protégées par seL4.
* Les identifiants entiers ou tables en mémoire ne confèrent aucune autorité noyau ; ils servent uniquement à la comptabilité et aux politiques internes de BAURA Core.
* Par défaut, l'accès aux ressources non déclarées échoue de manière fermée (*deny-by-default*).

### C. Charte Graphique et Expérience Visuelle — **MISE À JOUR COMPLÈTE**
* La marque BAURA OS repose sur **4 jetons de couleur immuables** (voir `GUI/Styles_composants_ui/icones/docs/01-DESIGN-SPEC.md`) :
  | Jeton | Clair | Sombre | Rôle |
  |---|---|---|---|
  | `b1` indigo | `#4F46E5` | `#6D66F2` | Barre du symbole, accent principal |
  | `b2` violet | `#8B5CF6` | `#9D77FF` | Demi-disque haut, dégradés |
  | `b3` lavande | `#C4B5FD` | `#CFC3FF` | Demi-disque bas, surfaces douces |
  | `b4` or | `#AB8E18` | `#D0AE2A` | Point doré, indicateur « app ouverte » |
  | `nuit` | `#0F0B2E` | — | Fonds très sombres |
* **Le symbole BAURA** n'est PAS un 'B pixelisé' — c'est un symbole vectoriel : barre indigo verticale (pilule) + demi-disque violet haut + demi-disque lavande bas + point doré (`viewBox = 150 29 199 420`).
* **⚠️ Important :** L'ancienne palette bleu/cyan (`#4A9EFF`, `#6BD5E1`) est **remplacée** par la charte indigo/violet/lavande/or. Toute référence à ces anciennes couleurs dans le code doit être mise à jour.
* Texte/interface : `fg=#17133D` clair / `#F4F2FF` sombre · `mut=#5F5A88` / `#B4B0DA` · verre avec `blur=36px, saturate=1.9`.
* Police : **Inter** (SemiBold 28 pour le splash) avec repli 5×7 bitmap freestanding. Terminal : **JetBrains Mono**.
* Au démarrage, le splash animé (≈ 2,4 s) enchaîne : fondu fond + halo → barre indigo → demi-disques → point doré (ressort) → « BAURA OS » + barre de progression réelle.
* Mode « réduire le mouvement » : dessin direct de l'état final du splash, durées d'animation à 0 ms partout.

### D. Invariants du Moteur Graphique BGE, du SDK BUI & de la Coque (Cahier des charges Nov 2026)
Conformément au *Cahier des charges de la pile graphique et du bureau* et au dossier `GUI/Styles_composants_ui` :
1. **Zéro coordonnées en dur dans les applications/widgets** : toute interface est un arbre de widgets calculé dynamiquement par un layout en 2 passes (`measure(available_dp)` -> `layout(rect_dp)`).
2. **Unités logiques (dp)** : coordonnées déclarées en dp ; le facteur d'échelle (1x, 1.25x, 1.5x, 2x) est appliqué au rendu.
3. **Le compositeur ne connaît aucun bouton** : il assemble des surfaces rectangulaires soumises via BSP (*Baura Surface Protocol*). La barre des tâches, le menu Démarrer et les fenêtres sont des clients distincts.
4. **Composition par régions d'endommagement (Damage Rects)** : seuls les rectangles modifiés sont recomposés (un rendu plein écran CPU 1280x720 coûtant ≈ 186 ms soit 5 i/s sans cache).
5. **Esthétique native (Document 14-NATIF)** : fenêtres à angles compacts (rayon 10 px), contrôles à 7 px, barre de titre native 34 px avec boutons pleine hauteur, verre dépoli réservé au châssis (Split-dock, menus, tray).
6. **Streaming VRAM freestanding** : flux RLE haute fidélité (`baura_bge_boot_data.h`) permettant au thread initial seL4 d'afficher le splash vectoriel et le bureau natif sans allocation dynamique.
7. **Modèle Applicatif Modulaire (SDK BUI)** : l'ensemble des composants d'interface (Dock, TopBar, BStart, Fichiers, Terminal) implémentent le descripteur d'application universel `bui_app_desc_t` avec cycle de vie standardisé (`init`, `build_ui`, `on_event`, `destroy`), garantissant la pérennité architecturale.
8. **Gestionnaire de Bureau Interactif (BDS/PS2)** : lecture des interruptions et paquets souris PS/2 sous seL4, curseur vectoriel 16x18 avec buffer de restauration 16x20 px, partitionnement sans chevauchement (Fichiers & Terminal avec 32 px d'écart), Drag & Drop fluide en temps réel et réorganisation de focus Z-order.

---

## 3. Cartographie Complète des Composants

```text
BAURA-OS/
├── BOOT/                                # Démarrage et firmwares
│   ├── FIRMWARE/
│   │   ├── FRAMEBUFFER/
│   │   │   ├── baura_boot_splash.*      # Moteur de splash freestanding C (dégradé Nuit, halos, logo vectoriel)
│   │   │   └── baura_inter_font.h       # ★ Police Inter cuite (SemiBold 28px + Regular 14px)
│   │   ├── UART/uart_driver.*           # Pilote série 16550 COM1
│   │   └── UEFI/uefi_mem.*              # Validation normalisée de la carte mémoire UEFI
│   ├── SEL4/
│   │   ├── initial_task.c               # Adaptateur x86_64 de la tâche initiale seL4 (mapping VRAM & splash)
│   │   ├── initial_task_start.S         # Point d'entrée assembleur AMD64 (_start)
│   │   ├── initial_task.ld              # Script de liaison ELF64 de la tâche initiale
│   │   └── grub.cfg                     # Configuration GRUB Multiboot2 avec support vidéo
│   ├── BOOTLOADER/                      # Contrats de handoff (KERNEL_HANDOFF.h, L4_LOADER.c)
│   ├── INSTALLER/                       # Interface graphique de l'installateur
│   └── tests/                           # Tests unitaires du sous-système BOOT
├── KERNEL/
│   ├── BAURA_CORE/                      # Services et gestionnaires BAURA Core
│   │   ├── SYSTEM_MANAGER/              # Séquence d'initialisation ordonnée et idempotente
│   │   ├── SYSTEM_API/                  # ABI v2 (JSON, codec IPC v2, baura_core_entry)
│   │   ├── STORAGE/                     # VFS canonique, gestionnaire de volumes et storage
│   │   ├── SECURITY/                    # Tables de capacités C et CSpaces Rust
│   │   ├── PROCESS_MANAGER/             # Gestionnaire de processus et cycle de vie
│   │   ├── RESOURCE_MANAGER/            # Quotas et comptabilité des ressources
│   │   ├── DEVICE_MANAGER/              # Registre d'inventaire PCI et probing
│   │   ├── HARDWARE_MANAGER/            # Gestion matérielle consolidée et reporting
│   │   ├── DRIVER_MANAGER/              # Contrat d'isolation de pilotes en Rust
│   │   ├── DRIVER_DOCS/                 # Documentation d'architecture des pilotes (01 à 09)
│   │   ├── EVENT_BUS/                   # Bus d'événements synchrone
│   │   ├── LOGGING/                     # Journal circulaire chronologique
│   │   ├── FAULT_ENGINE/ & RECOVERY/    # Gestion et remédiation des fautes
│   │   ├── NETWORK/                     # Façade d'émission de paquets
│   │   └── tests/                       # Tests unitaires des services et de l'ABI
│   └── L4/
│       ├── sel4/                        # Arbre officiel du micro-noyau seL4 16.0.0-dev
│       └── arch/ & MEMORY/              # Fragments et roadmaps microkernel
├── GUI/                                 # Sous-système graphique BGE — considérablement enrichi
│   ├── UI_V2/                           # ★ NOUVEAU : Pile de composants haute fidélité native
│   │   ├── baura_vector.h               # Rendu 2D vectoriel SDF anti-crénelé
│   │   ├── baura_text.h                 # Moteur BGF (Inter, JetBrains Mono, RTL arabe)
│   │   ├── baura_glass.h                # Verre dépoli (flou box 3 passes, saturation, reflets)
│   │   ├── baura_motion.h / _wm_motion  # Physique d'animation par ressorts SPRING
│   │   ├── baura_theme_brand.h          # Charte officielle (Indigo/Violet/Lavande/Or/Nuit)
│   │   ├── baura_ui.h / _textedit       # Toolkit de composants interactifs
│   │   ├── icons_data.h                 # 550 icônes système vectorielles
│   │   ├── appicons_data.h              # 122 icônes d'applications BAURA
│   │   ├── inter.bgf / inter_bgf_data.h # Données binaires C de la police Inter (3,16 Mo)
│   │   ├── mono.bgf / mono_bgf_data.h   # Données binaires JetBrains Mono (409 Ko)
│   │   ├── baura_shell.h                # Coque de bureau (barre Split-Dock, tray pill, fond)
│   │   └── baura_apps.h                 # Applications système (Fichiers, Terminal)
│   ├── BGE/                             # Renderer logiciel ARGB8888, effets de flou, 3D
│   ├── UI_FRAMEWORK/                    # Toolkit historique, brand, taskbar, systembubble
│   │   ├── baura_brand.h                # Symbole vectoriel BAURA (à migrer vers nouveau symbole)
│   │   ├── baura_theme.h                # Jetons couleur (à pointer vers baura_theme_brand.h)
│   │   ├── baura_hybrid_taskbar.*       # Barre des tâches coupée en deux (défaut)
│   │   ├── system_bubble.*              # Bulles système avec graphique de charge
│   │   ├── papirus_icon_catalog.*       # Catalogue d'icônes (à relier à icons_data.h/550)
│   │   └── ui_extensions.*              # Extensions UI
│   ├── COMPOSITOR/ & DISPLAY_SERVER/    # Composition et gestion des surfaces d'affichage
│   ├── WINDOW_MANAGER/ & DESKTOP/       # Fenêtrage, focus et shell de bureau
│   ├── INPUT/                           # Pilote PS/2 (clavier et souris)
│   └── PLATFORM/                        # Service GUI dans BAURA Core et runtime framebuffer
│   │
│   └── Styles_composants_ui/            # DOCUMENTATION DE RÉFÉRENCE VISUELLE (14 chapitres)
│       ├── baura-os-docs/               # ★ NOUVEAU : référence C++ complète contre BGE existant
│       │   ├── 00-AUDIT-GUI/            # Audit écarts actuels → cible
│       │   ├── 01-IDENTITE-ET-VECTEUR/  # Charte indigo/violet/lavande/or, symbole vectoriel, AA
│       │   ├── 02-TEXTE/                # Police Inter cuite (inter.bgf ≈ 500 Kio), accents, crénage
│       │   ├── 03-VERRE-ET-OMBRES/      # Verre complet : ombre → flou → saturation → teinte → bord → reflet
│       │   ├── 04-COMPOSANTS/           # Kit complet : champs, listes, onglets, interrupteurs, menus
│       │   ├── 05-MOUVEMENT/            # Animations de fenêtre, ressorts cubicbezier(SPRING)
│       │   ├── 06-DEMARRAGE/            # Splash animé 2,4 s, framebuffer Multiboot2/seL4 (conception)
│       │   ├── 07-BUREAU-CIBLE/         # 15 écrans coque + 12 apps — bureau par défaut = barre coupée en deux
│       │   ├── 08-LANGUES-ET-ACCESSIBILITE/ # Arabe/RTL + contraste WCAG + arbre a11y
│       │   ├── 09-ICONES/               # 550 icônes : 97 BAURA + 453 Lucide (ISC) + CATALOGUE.md
│       │   ├── 10-FEUILLE-DE-ROUTE/     # P0–P15 : de l'état actuel à la cible (≈ 40 sem-personne)
│       │   ├── 11-ICONES-APPLICATIONS/  # 122 icônes d'apps BAURA (grille 64×64, dégradé, point doré)
│       │   ├── 12-BACKEND/              # Backend Rust (34 tests) + ABI C + enveloppe C++
│       │   ├── 13-MONITEUR/             # Gestionnaire des tâches : 3 pages, tri par impact, « Terminer »
│       │   └── 14-NATIF/                # Règles d'apparence native : barre 34 px, verre réservé au châssis
│       ├── icones/                      # ★ NOUVEAU : moteur BGE C++ de référence autonome (CMake)
│       │   ├── bge/include/bge/         # core.hpp, gfx.hpp, scene.hpp, components.hpp, icons_data.hpp
│       │   ├── bge/examples/            # desktop_demo.cpp (bureau complet → PPM)
│       │   ├── bge/ports/               # bge_core.rs (Rust) + bge_tokens.py (Python)
│       │   ├── bge/tools/gen_icons.py   # baura-icons.js → icons_data.hpp
│       │   ├── icones/                  # SVG et PNG de l'icône d'app BAURA (180–1024 px)
│       │   └── logos/                   # Jeu complet de logos BAURA (SVG + PNG)
│       ├── logos/                       # Copie du jeu de logos à la racine Styles_composants_ui
│       ├── fenetre.html                 # Prototype HTML fenêtre
│       └── taskbar-and-desktop-tools-ui-baura-os.html  # Prototype HTML bureau
├── COMMUNICATION/                       # Bus de messages en mémoire et RPC synchrone
├── USERSPACE/                           # Applications (Task Manager, Terminal, Explorer, etc.)
├── DEVELOPER_PLATFORM/                  # SDK BAURA C (baura_sdk.h)
└── PACKAGE_FORMAT/                      # Spécification du format de paquet AESB
```

---

## 4. Chaîne de Démarrage Cible (x86_64 Boot Flow)

```text
[Firmware UEFI / BIOS]
          │
          ▼
[GRUB 2 — Multiboot2]
  - Active le mode vidéo linéaire (1024x768x32 ou auto via efi_gop / vbe)
  - Conserve le framebuffer (set gfxpayload=keep)
  - Charge le noyau : multiboot2 /boot/kernel.elf
  - Charge le module initial : module2 /boot/baura_init.elf baura-init
          │
          ▼
[seL4 Microkernel (pc99 / x86_64)]
  - Initialise l'architecture, la pagination 64 bits et les interruptions
  - Reçoit les informations Multiboot2 (dont MULTIBOOT2_TAG_FB)
  - Crée les objets racines et assemble le seL4_BootInfo
  - Injecte le chunk SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER dans extraLen
  - Lance la tâche initiale utilisateur en passant le BootInfo dans RDI
          │
          ▼
[Tâche Initiale BAURA — baura_sel4_initial_task]
  - Valide la structure et les régions de slots de seL4_BootInfo
  - Analyse extraLen pour extraire les métadonnées vidéo (addr, pitch, w, h, bpp)
  - Trace la détection vidéo sur le port série COM1 (seL4_DebugPutChar)
  - Fournit le hook start_graphical_service à la plateforme Core
  - Appelle baura_core_initial_task_start(&platform)
          │
          ▼
[BAURA Core — baura_core_initial_task_start]
  - Initialise les services de base (Config, Log, EventBus, Quotas)
  - Invoque start_graphical_service :
       → Exécute baura_boot_draw_splash()
       → Dessine le dégradé, le panneau en verre, l'emblème BAURA et la barre
  - Émet sur la console série : "BAURA OS Core ready"
          │
          ▼
[Services Utilisateurs Isolés & Shell Graphique BGE]
```

---

## 5. Matrice de Maturité des Composants

| Domaine | Niveau de Réalisation | Statut Actuel |
| :--- | :--- | :--- |
| **Boot Splash 2D** | Opérationnel V2 (Animé & Dynamique) | Moteur freestanding C réécrit sans dessin manuel : police Inter SemiBold 28px cuite (`baura_inter_font.h`), animation cadencée de 0 à 100% avec mise à jour rapide sans scintillement (`baura_boot_update_progress`). |
| **Bureau BGE Live (QEMU)** | Opérationnel en VRAM seL4 | Bascule automatique dès la fin du boot vers le Bureau officiel complet (`baura_boot_draw_desktop`) : Split-Dock, Top-Tray, fenêtre Fichiers et fenêtre Terminal avec prompt et curseur clignotant. |
| **Boot x86_64 seL4** | Opérationnel & Validé | Kernel seL4 pc99 x86_64 + Multiboot2 Tag 5 + découpage logarithmique d'untyped + pagination LargePages 2MB à `0x20000000`. |
| **ISO & QEMU** | Opérationnel | Image hybride bootable `build/baura-os.iso` générée avec succès via `grub-mkrescue` ; exécution directe par `make run-baura`. |
| **BAURA Core** | Opérationnel (Archive C) | Services de base, System Manager et Hardware Manager compilables en `libbaura_core.a`. |
| **Wire ABI v2** | Spécifié & Testé | JSON normatif, codec IPC v2 validés sur l'hôte. Transport seL4 à brancher. |
| **BGE Graphics Engine & UI_V2** | Opérationnel (UI_V2 Déployé & Live) | Arborescence `GUI/UI_V2/` active. Rendus validés sur l'hôte et capture screendump VRAM QEMU extraite (`qemu_live_desktop_rendered.png`). |
| **BGE Référence (baura-os-docs)** | Référence C++ complète | Moteur autonome `bge/` avec 14 chapitres docs, 550 icônes, backend Rust 34 tests. Compilé sur hôte. |
| **Charte visuelle** | Définie & Rendu Conforme | Indigo `#4F46E5`, Violet `#8B5CF6`, Lavande `#C4B5FD`, Or `#D0AE2A`, Nuit `#120F3A`/`#0F0B2E` appliqués rigoureusement dans `baura_theme_brand.h`, dans le splash et sur le bureau. |
| **Icônes** | 550 définies | 97 BAURA + 453 Lucide (ISC). Embarquées dans `GUI/UI_V2/icons_data.h`. |
| **Icônes d'applications** | 122 définies | Grille 64×64, dégradé, point doré. Embarquées dans `GUI/UI_V2/appicons_data.h`. |
| **Backend Rust GUI** | 34 tests OK (hôte) | `baura-prefs`, `baura-sysmon`, `baura-icons`, `baura-shell`, `baura-i18n`, `baura-ffi`. À raccorder à BAURA Core. |
| **Pilotes Matériels** | Opérationnel (PCI v2) | Sous-système PCI seL4 avec port I/O cap sans collision. Détection automatique de 6 périphériques sous QEMU. BAR sizing & probe testés. |
| **VFS & Stockage** | Spécifié & Abstrait | Chemins canoniques opérationnels ; préparation du pilote VirtIO-blk (lecture seule). |

---

## 6. Feuille de Route GUI (P0–P15) — Source : `10-FEUILLE-DE-ROUTE/README.md`

Migration de l'état actuel du dépôt vers la cible visuelle des docs 01–14. **Chemin critique ≈ 40 semaines-personne.**

| Phase | Objectif | Effort | Risque | Statut |
|---|---|---|---|---|
| **P0** | Fondations : `GUI/UI_V2/`, CI locale, bibliothèques communes (Vector, Text, Glass, Motion) | 0,5 sem | faible | ✅ **Terminé** |
| **P1** | Identité : thème indigo/violet/lavande/or, symbole vectoriel pur, 550 icônes | 1 sem | faible | ✅ **Terminé** |
| **P2** | Texte : Inter cuite (`baura_inter_font.h` 28px/14px + `inter_bgf_data.h`), accents, crénage | 1 sem | moyen | ✅ **Terminé (Boot & UI_V2)** |
| **P3** | Verre & ombres complets (+ cache fond flouté dès cette phase) | 1,5 sem | moyen | 🔶 Intégré (`baura_glass.h`) |
| **P4** | Composants complets : champs, listes, onglets, interrupteurs | 3 sem | moyen | 🔶 Intégré (`baura_ui.h`) |
| **P5** | Mouvement : animations fenêtre, ressorts (`baura_motion.h`, `baura_wm_motion.h`) | 1,5 sem | moyen | 🔶 Intégré |
| **P6** | Coque : **barre coupée en deux (défaut)**, Démarrer, recherche, centre d'actions | 3 sem | **élevé** | ✅ Rendu live VRAM QEMU validé |
| **P7** | Connexion & installateur : refonte rendu | 2 sem | moyen | ⬜ Prévu |
| **P8** | 12 applications : Fichiers, Paramètres, Terminal, Moniteur… | 6 sem | **élevé** | ✅ Rendu Fichiers & Terminal live QEMU |
| **P9** | Démarrage & Bureau QEMU : animation progressive + transition bureau live | 2 sem | **élevé** | ✅ **Terminé & Validé QEMU** |
| **P10** | Langues & accessibilité : RTL, arabe, WCAG, clavier | 3 sem | moyen | ⬜ Prévu |
| **P11** | Performance : cache, rectangles sales, SIMD → 60 i/s | 3 sem | **élevé** | ⬜ Prévu |
| **P12** | Icônes apps : illustrateur + ~200 rôles (122 intégrées dans `appicons_data.h`) | 2 sem | faible | 🔶 122 intégrées |
| **P13** | Backend Rust : prefs VFS, recherche, bascules, énergie | 5 sem | moyen | 🔶 Backend 34 tests OK (hôte) |
| **P14** | Moniteur : source BAURA Core, terminaison réelle | 1,5 sem | moyen | ⬜ Prévu |
| **P15** | Mode natif : cadre compact, barre menus, liste détails | 4 sem | moyen | ⬜ Prévu |

**Bureau par défaut = barre coupée en deux (`split-dock`).** Autres dispositions = options Paramètres.

> ⚠️ P11 (performance) : bureau complet 1280×720 actuel ≈ 186 ms (≈ 5 i/s) sans cache. Objectif = 60 i/s.

---

## 7. Architecture BGE Référence (moteur autonome)

Le moteur `GUI/Styles_composants_ui/icones/bge/` est un BGE **autonome de référence** (sans dépendance, C++17, CMake) :

```
Pipeline d'une image :
entrées → Engine::dispatch → Widget::onEvent
horloge → Animator::update(dt)
         → 1. MESURE → 2. PLACEMENT → 3. DESSIN (ordre Z) → 4. PRÉSENT
```

**Primitives** : `fillRRect` (SDF anti-crénelé), `shadow`, `glass` (flou+saturation+teinte+bord+reflet), `fillPath`/`strokePath` (parseur SVG), `icon`, `logo`.

**Mise en page** : flex simplifié (`dir`, `align`, `justify`, `gap`, `pad`, `grow`, `absolute`).

**Verre** (ordre strict) : ombre portée → copie zone → flou 3 passes box blur (σ=18) → saturation ×1,9 → teinte `glass` → bordure 1 px → reflet haut.

**Mouvement** : `Tween{from,to,dur,ease,apply}` + `SPRING = cubic-bezier(.3,1.45,.45,1)`. `reduce_motion` = durées → 0.

Compile : `cd bge && g++ -std=c++17 -O2 -Iinclude examples/desktop_demo.cpp -o demo && ./demo`

---

## 8. Invariants GUI Supplémentaires

* **Bureau par défaut** : barre coupée en deux (`BauraShell::Layout::SplitDock`) — gauche : Démarrer+recherche+apps ; droite : réseau, batterie, langue, date. Les autres layouts sont des options.
* **Verre réservé au châssis** : barre des tâches, Démarrer, menus, bulles — les fenêtres d'applications ont du **matériau plein** (règle native, doc 14).
* **Icônes d'applications tierces** : BAURA ne reproduit pas les logos de marques. Les apps connues reçoivent l'icône de leur *rôle* (firefox → `browser`). Option « Icônes d'origine » désactive ça.
* **Terminologie** : on dit « Terminer » (pas « Kill »). La confirmation dit ce qui sera libéré. Système = protégé (cadenas).
* **Contraste** : 9 paires sous seuil WCAG identifiées (doc 08). Correctifs connus à appliquer en P10.
* **Texte arabe** : `visual()` avant dessin, classes RTL (R), ligatures lam-alef, parenthèses miroir — pas d'UAX #9 complet.

---

## 9. Synchronisation et Règles de Maintenance

* Toute modification substantielle de code, ajout de fonctionnalité ou passage d'étape dans la roadmap entraîne **immédiatement** la mise à jour conjointe de :
  1. [`Gemini agy actions.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/Gemini%20agy%20actions.md) (journal chronologique, détails des commits logiques, cases cochées).
  2. [`Gemini agy context.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/Gemini%20agy%20context.md) (état global, matrice de maturité, flux architectural).
  3. [`BAURA_TEST_JOURNAL.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/BAURA_TEST_JOURNAL.md) (résultats de tests, erreurs et corrections).
  4. [`nextstep agy.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/nextstep%20agy.md) (instructions de compilation et diagnostic d'exécution pas-à-pas).
  5. Dossier de documentation des pilotes : `KERNEL/BAURA_CORE/DRIVER_MANAGER/Documentation importante/BAURA_drivers_complet_v2/` (guides 01 à 09, matrice de compatibilité, dimensionnement BAR et backend PCI seL4).

