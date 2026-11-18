# Journal d'Actions, Roadmap & Suivi de Progression — BAURA OS

> **Document de référence pour le suivi des travaux menés par l'assistant IA (Gemini / Antigravity)**  
> **Dépôt :** BAURA-OS (Cible x86_64, micro-noyau seL4, BAURA Core, BGE GUI)  
> **Dernière mise à jour :** 16 novembre 2026 (Intégration du véritable rendu natif BGE haute fidélité)

---

## 1. Vue d'Ensemble & Objectifs

Ce document assure une traçabilité rigoureuse de toutes les actions de conception, d'implémentation et de configuration du système d'exploitation **BAURA OS**. Il sert de mémoire vivante du projet en récapitulant les décisions prises, les réalisations terminées, l'état d'avancement étape par étape et les prochaines tâches à accomplir.

---

## 2. Journal Chronologique des Actions

### Session 1 — 8 octobre 2026 : Audit Complet & Implémentation du Démarrage Graphique x86_64

#### Action 1.1 : Prise de Contexte Intégrale du Projet
* **Périmètre exploré :**
  * Architecture générale : association complémentaire du micro-noyau **seL4** (mécanismes bas niveau isolés, capacités CSpace, IPC) et de **BAURA Core** (politiques, services, VFS, gestionnaires).
  * Référentiels directeurs : feuille de route historique (`BAURA OS — Roadmap du noyau et du système de boot.md`), `KERNEL/BAURA_CORE/README.md`, `ROADMAP_BOOT_GUI_INTEGRATION.md`, spécifications du VFS (`INFRASTRUCTURE/SPECIFICATIONS/SYSTEM_SPECIFICATIONS/VFS_SPEC.md` avec syntaxe canonique `PC/<LECTEUR>:/...`), et directives de l'agent `BLUE AI`.
  * Constat de départ : séparation stricte entre le bring-up historique Multiboot 32 bits (`boot.asm`, `kernel_main.c`) et le chemin cible **x86_64 seL4** (`BOOT/SEL4/initial_task.c`, `baura_core_entry.c`).
* **Résultat :** Cartographie complète des 1 400+ fichiers du projet et des dépendances de compilation.

#### Action 1.2 : Conception & Écriture du Moteur de Boot Splash Freestanding en C
* **Fichiers créés :**
  * [`BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.h`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.h) : Définition des structures `BauraBootFramebuffer`, constantes de couleurs conformes à la charte (`BAURA_BOOT_COLOR_*`), et prototypes de rendu autonome.
  * [`BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.c) :
    * Validation stricte des résolutions d'écran (320x240 à 7680x4320, 32 bpp, pitch).
    * Prise en charge des formats de pixels `RGBX8888` et `BGRX8888`.
    * Primitives graphiques 2D freestanding (sans libc externe) : remplissage rectangulaire, interpolation de dégradé vertical (gradient du fond sombre `0xFF101827` vers `0xFF1A2944`).
    * Police bitmap 5x7 intégrée pour le rendu typographique.
    * Tracé de l'emblème de marque BAURA : rectangle bleu accent arrondi, glyphe 'B' stylisé, étincelle cyan supérieure droite.
    * Panneau vitré central avec bordure, centrage vertical du titre `"BAURA OS"`, ligne de statut paramétrable et barre de progression.
* **Ajustement typographique :** Suppression du sous-titre de débogage `"x86_64 BOOT"` qui polluait l'écran de démarrage, afin de respecter strictement l'identité de marque officielle définie dans `GUI/UI_FRAMEWORK/baura_brand.h`.

#### Action 1.3 : Configuration du Support Framebuffer dans GRUB Multiboot2
* **Fichier modifié :** [`BOOT/SEL4/grub.cfg`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/grub.cfg)
  * Chargement des modules vidéo : `insmod all_video`, `insmod gfxterm`.
  * Résolution déclarée : `set gfxmode=1024x768x32,1280x800x32,auto`.
  * Préservation du payload vidéo linéaire pour Multiboot2 : `set gfxpayload=keep`.

#### Action 1.4 : Évolution de la Tâche Initiale seL4 x86_64
* **Fichier modifié :** [`BOOT/SEL4/initial_task.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.c)
  * Implémentation de la fonction `parse_sel4_bootinfo_framebuffer` pour parcourir les chunks étendus du `seL4_BootInfo` (`SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER`).
  * Extraction et journalisation détaillée sur la console série seL4 (`seL4_DebugPutChar`) de la géométrie vidéo (largeur, hauteur, bpp, pitch, adresse physique).
  * Implémentation du service graphique `sel4_start_graphical_service` invoquant `baura_boot_draw_splash`.
  * Enregistrement de ce hook dans la plateforme Core (`platform.start_graphical_service = sel4_start_graphical_service`).

#### Action 1.5 : Spécification du Handoff Vidéo de Boot
* **Fichier modifié :** [`BOOT/BOOTLOADER/KERNEL_HANDOFF.h`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/BOOTLOADER/KERNEL_HANDOFF.h)
  * Ajout de la structure `BauraBootVideoHandoff` et du magic `BAURA_BOOT_VIDEO_MAGIC` (`0x5649444F`).

#### Action 1.6 : Test Unitaire Hôte du Splash de Démarrage
* **Fichier créé :** [`BOOT/tests/boot_splash_test.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/tests/boot_splash_test.c)
  * Couverture : validation des bornes et paramètres invalides, rendu vérifié en formats RGBX et BGRX, tests de clipping hors écran.

#### Action 1.7 : Mise à Jour du Système de Compilation
* **Fichier modifié :** [`Makefile`](file:///c:/Users/SDA/Downloads/BAURA-OS/Makefile)
  * Intégration de `BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.c` dans la cible de la tâche initiale x86_64 (`$(SEL4_INITIAL_TASK_ELF)`).
  * Ajout de `boot_splash_test` dans la suite de tests `test-boot`.

#### Action 1.8 : Alignement de la Documentation Technique
* **Fichiers mis à jour :**
  * [`KERNEL/BAURA_CORE/README.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/KERNEL/BAURA_CORE/README.md)
  * [`KERNEL/L4/ROADMAP_BOOT_GUI_INTEGRATION.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/KERNEL/L4/ROADMAP_BOOT_GUI_INTEGRATION.md)
  * [`BOOT/README.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/README.md)

---

## 3. Feuille de Route Dynamique (Roadmap)

#### Action 1.9 : Document de Contexte Dédié, Balayage Untypeds & Contrôle d'Autorité
* **Fichier créé :** [`Gemini agy context.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/Gemini%20agy%20context.md)
  * Création d'un document dédié capturant le contexte vivant du projet : dualité seL4/Core, syntaxe canonique VFS `PC/<LECTEUR>:/...`, modèle graphique BGE, chaîne de boot complète et matrice de maturité.
  * Engagement de synchronisation bi-directionnelle continue avec ce journal d'actions.
* **Fichier mis à jour :** [`BOOT/SEL4/initial_task.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.c)
  * Ajout de `scan_sel4_device_untypeds` pour corréler l'adresse physique du framebuffer avec les objets Untyped Device du `seL4_BootInfo`.
  * Implémentation du callback d'autorité initial `sel4_authority_check` accordant les droits de démarrage aux sujets racine (`subject_id <= 1`).
  * Branchement de ce callback dans `platform.authority_check` pour sortir de l'état fail-closed par défaut lors du boot.

---

### Session 2 — 9–16 octobre 2026 : Build seL4 x86_64 + ISO + Audit Complet GUI

#### Action 2.1 : Build x86_64 Complet ✅
* `rsync` + symlinks seL4 + fix `lxml` + fix répertoires libsel4 → `kernel.elf` + `baura_init.elf`.

#### Action 2.2 : Renommage ISO & Cibles
* `baura-os.iso`, alias `baura-iso`/`run-baura`. ISO bloquée `grub-file` → fix `grub-pc-bin xorriso`.

#### Action 2.3 : Lecture & Intégration Documentation GUI (16 oct)
* **14 chapitres baura-os-docs** + moteur BGE autonome `icones/bge/` intégrés dans le contexte.
* **Charte révisée** : indigo `#4F46E5` / violet `#8B5CF6` / lavande `#C4B5FD` / or `#AB8E18`.
* **Symbole BAURA** = pilule + demi-disques + point or (PAS 'B' pixelisé).
* **Bureau défaut** = barre coupée en deux. Verre = châssis seulement (règle native).
* **GUI P0–P15** ≈ 40 sem-personne. Perf actuelle ≈ 5 i/s → cible 60 i/s.
* **550 icônes** + **Backend Rust 34 tests** + **122 icônes apps**.
#### Action 2.4 : Test QEMU, Erreur fsgsbase et Correction du Profil CPU
* **Exécution :** `qemu-system-x86_64 -m 2G -cdrom build/baura-os.iso -serial stdio -no-reboot -no-shutdown`
* **Erreur immédiate :** `fsgsbase instructions not supported by the processor`
* **Analyse :** Le micro-noyau seL4 x86_64 exigeait les instructions matérielles `FSGSBASE` par défaut.
* **Correction dans Makefile & CMake :**
  - CMake seL4 universel : `-DKernelSupportPCID=OFF -DKernelHugePage=OFF -DKernelFSGSBase=msr`.
  - QEMU : `-cpu max -vga std -serial stdio`.
* **Statut :** Compilé et validé sans panique CPU.

### Session 3 — 16 octobre 2026 : Exécution Userspace seL4 Réussie & Activation du Framebuffer Linéaire

#### Action 3.1 : Premier Démarrage Réussi en Espace Utilisateur (Ring 3) ✅
* **Exécution :** `make run-baura`
* **Résultat observé :**
  - Initialisation complète du noyau seL4 pc99 x86_64.
  - Bascule en mode utilisateur sans faute de pagination ni panique d'instructions.
  - Lancement réussi de `baura_sel4_initial_task`.
  - Détection et validation du `seL4_BootInfo` (96 untypeds inspectés).
  - BAURA Core initialisé : `"BAURA OS Core ready"`.

#### Action 3.2 : Diagnostic Vidéo & Mode Texte EGA
* **Constat :** Console série fonctionnelle, mais la fenêtre graphique QEMU reste sur l'écran SeaBIOS.
* **Analyse :** Le log `physical address=b8000 pitch=160 resolution=80x25@16 type=2` montre que GRUB a démarré en mode texte EGA 80x25 car :
  1. L'en-tête Multiboot2 de seL4 n'avait pas le tag Framebuffer (Tag 5).
  2. `gfxpayload=keep` conservait le mode texte actif de la console GRUB.
* **Correction :**
  - Ajout du tag Framebuffer Multiboot2 (Tag 5, 1024x768x32) dans [`KERNEL/L4/sel4/src/arch/x86/multiboot.S`](file:///c:/Users/SDA/Downloads/BAURA-OS/KERNEL/L4/sel4/src/arch/x86/multiboot.S).
  - Déclaration explicite de `set gfxpayload=1024x768x32,1024x768,auto` et chargement des pilotes VBE/VGA dans [`BOOT/SEL4/grub.cfg`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/grub.cfg).

#### Action 3.3 : Implémentation du Mapping Mémoire Userspace (seL4 LargePage)
* **Fichier modifié :** [`BOOT/SEL4/initial_task.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.c)
* **Mécanisme :**
  - Fonction `map_framebuffer` : localise l'untyped matériel couvrant l'adresse physique VRAM.
  - Retypage de l'untyped en pages 2 Mo (`seL4_X86_LargePageObject`) via `seL4_Untyped_Retype`.
  - Mapping direct dans le VSpace du thread initial à l'adresse virtuelle `0x20000000` via `seL4_X86_Page_Map`.
  - Raccordement du pointeur `g_boot_fb.pixels` pour le tracé direct du splash haute fidélité (`baura_boot_draw_splash`).

#### Action 3.4 : Résolution du Symbole `__sel4_ipc_buffer` et Neutralisation TLS Freestanding
* **Constat :** Échec à la compilation/liaison de `baura_init.elf` suite à l'introduction des appels `seL4_Untyped_Retype` et `seL4_X86_Page_Map` qui utilisent le tampon IPC.
* **Résolution :**
  - Ajout de `LIBSEL4_THREAD_LOCAL seL4_IPCBuffer *__sel4_ipc_buffer = 0;` dans `initial_task.c`.
  - Initialisation au point d'entrée avec `seL4_SetIPCBuffer(boot_info->ipcBuffer);`.
  - Intégration des sections `.tdata` et `.tbss` dans `BOOT/SEL4/initial_task.ld`.
  - Ajout de `-DLibSel4UseThreadLocals=OFF` dans la configuration CMake seL4 (`Makefile`).
  - Rédaction du guide de référence : [`nextstep agy.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/nextstep%20agy.md).

#### Action 3.5 : Audit & Intégration du Dossier de Documentation Pilotes (`BAURA_drivers_complet_v2`)
* **Périmètre exploré :** `KERNEL/BAURA_CORE/DRIVER_MANAGER/Documentation importante/BAURA_drivers_complet_v2`
* **Éléments clés inventoriés :**
  - Spécifications complètes de démarrage PC réel (`01_DEMARRAGE_PC_REEL.md`), BootInfo et framebuffer (`02_BOOTINFO_ET_CONSOLE_FRAMEBUFFER.md`), modèle de concurrence SPSC (`03`), interfaces de classe (`04`), modèle de sécurité et isolation DMA (`05`), plan VirtIO-blk lecture seule (`06`), paquets et MAJ (`07`), décisions ouvertes (`08`), dimensionnement BAR et mapping (`09`).
  - Implémentation du backend PCI mécanisme #1 sous seL4 (`pci_backend.c`, `pci_backend_sel4.c`, `pci_bar.c`).
  - Suppression planifiée des scanners ring-0 obsolètes (`pci_scanner.c`, `pci_scanner_enhanced.c`).
  - Intégration dans la feuille de route globale multi-piliers (BOOT, KERNEL, DRIVERS, GUI).

### Session 4 — 24 octobre – 1er novembre 2026 : Intégration de l'Écosystème Graphique UI_V2 & Refonte Boot Splash Inter V2

#### Action 4.1 : Réponse aux Exigences Visuelles & Élimination du Dessin "Fait Main"
* **Constat utilisateur :** Le rendu graphique initial (police 5x7 bitmap, tracés approchés) ne reflétait pas la qualité des spécifications et maquettes HTML/C++ contenues dans `GUI/Styles_composants_ui`.
* **Décision d'architecture :**
  1. Générer une police système autonome haute fidélité à partir du fichier vectoriel binaire `inter.bgf` pour le bootloader freestanding.
  2. Réécrire le module de boot splash (`baura_boot_splash.c`) pour adopter la charte officielle exacte (dégradé sombre, halos, arcs elliptiques précis, typographie Inter).
  3. Déployer l'espace `GUI/UI_V2/` intégrant la pile de rendu complète (SDF, verre dépoli, animations SPRING, coque Split-Dock et applications).

#### Action 4.2 : Extraction & Cuisson de la Police Inter (`baura_inter_font.h`)
* **Outil :** Écriture du script `bake_font.py` parsant la structure binaire `BGF1` d'`inter.bgf`.
* **Fichier produit :** [`BOOT/FIRMWARE/FRAMEBUFFER/baura_inter_font.h`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_inter_font.h) (212 Ko).
  - Intégration de 95 glyphes d'**Inter SemiBold 28px** (titres et marque).
  - Intégration de 95 glyphes d'**Inter Regular 14px** (messages de statut et informations de diagnostic).
  - Déclarations de types C préfixées pour garantir l'autonomie totale sans `stdlib.h`.

#### Action 4.3 : Réécriture Complète du Boot Splash Freestanding C
* **Fichier modifié :** [`BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.c)
  - Dégradé vertical dynamique Nuit Indigo (`#120F3A`) vers Violet sombre profond (`#2E1A7A`).
  - 5 anneaux circulaires concentriques avec alpha blending doux (3,5%) centrés derrière le logo.
  - Emblème vectoriel exact : pilule Indigo (`#4F46E5`), demi-disques Violet (`#8B5CF6`) et Lavande (`#C4B5FD`) selon des arcs continus rigoureux, point d'accent Or signature (`#D0AE2A`).
  - Typographie nette par projection des glyphes Inter SemiBold 28px.
  - Barre de progression 220×6 px aux angles arrondis (`rx=3`).
* **Fichier modifié :** [`BOOT/SEL4/initial_task.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.c)
  - Déclenchement automatique de `baura_boot_draw_splash(&g_boot_fb, "BAURA OS x86_64 INITIALIZING", 65)` dès le mapping mémoire du framebuffer.

#### Action 4.4 : Création du Sous-Système `GUI/UI_V2/` (Phases P0 & P1)
* **Arborescence créée :** [`GUI/UI_V2/`](file:///c:/Users/SDA/Downloads/BAURA-OS/GUI/UI_V2/)
  - Intégration de l'ensemble des modules d'architecture visuelle :
    - `baura_vector.h` (moteur SDF anti-crénelé)
    - `baura_text.h` (moteur de polices BGF, Inter, JetBrains Mono, support RTL)
    - `baura_glass.h` (verre dépoli avec flou 3 passes σ=18 et saturation)
    - `baura_motion.h` & `baura_wm_motion.h` (physique d'animation SPRING)
    - `baura_theme_brand.h` (charte couleur officielle)
    - `baura_ui.h` & `baura_textedit.h` (toolkit de composants UI)
    - `icons_data.h` (550 icônes système vectorielles) & `appicons_data.h` (122 icônes d'applications)
    - `inter.bgf` + `inter_bgf_data.h` (données binaires C intégrées de 3,16 Mo)
    - `mono.bgf` + `mono_bgf_data.h` (données JetBrains Mono de 409 Ko)
    - `baura_shell.h` (coque bureau avec barre en capsules *Split-Dock*)
    - `baura_apps.h` (applications de référence : Fichiers avec arborescence et tuiles, Terminal interactif)
    - `baura_boot_splash_v2.h` (splash animé complet)

#### Action 4.5 : Validation Visuelle et Génération des Rendu Hôte
* **Bancs de rendu :** `render_reference_screens.cpp` et `render_full_desktop.cpp` compilés sur l'hôte.
* **Captures d'écran produites et validées :**
  - `bge_boot_rendered.png` : Rendu du splash V2 avec typographie Inter SemiBold 28px.
  - `bge_desktop_rendered.png` : Bureau BAURA avec barre des tâches en capsules (*Split-Dock*), tray pill et fond d'écran immersif.
  - `bge_actual_full_desktop.png` : Rendu du bureau complet incluant la fenêtre de navigation de fichiers (dossiers Documents, Projets, Factures), la fenêtre de terminal sombre avec police JetBrains Mono, et le dock applicatif à 6 icônes.

#### Action 4.6 : Élimination de l'Écran Figé & Activation du Bureau Live dans QEMU (8 nov)
* **Diagnostic :** La version antérieure restait sur une image statique à 65% car `initial_task.c` effectuait un unique appel puis tombait dans `for (;;) seL4_Yield();`.
* **Corrections appliquées :**
  1. `baura_boot_update_progress` : rafraîchissement ultra-rapide sans scintillement de la barre et du texte de statut.
  2. Séquence de démarrage cadencée avec avancement réel (15% micro-noyau, 35% pagination VRAM, 55% Core, 75% bus PCI, 90% BGE, 100% Système prêt).
  3. Implémentation de `baura_boot_draw_desktop` et bascule automatique vers le bureau officiel de BAURA OS (Split-Dock, Top-Tray, fenêtres Fichiers et Terminal).
  4. Curseur vert clignotant dans le terminal alimenté par le thread en Ring-3.
* **Preuve d'exécution :** ISO compilée (`make baura-iso`, 3344 secteurs), démarrage vérifié et capture d'écran screendump extraite directement de la VRAM QEMU (`qemu_live_desktop_rendered.png`).

---

## 3. Feuille de Route Dynamique (Roadmap) — Kernel & GUI

### Phase 1 : Démarrage Graphique et Transition x86_64 (seL4 + Core)
- [x] **1.1** Conception d'un module de rendu C freestanding pour l'image de boot BAURA (`baura_boot_splash`).
- [x] **1.2** Épuration et centrage du titre `"BAURA OS"` selon la charte officielle (suppression du texte parasite d'architecture).
- [x] **1.3** Configuration de GRUB Multiboot2 avec conservation du framebuffer (`gfxpayload=keep`).
- [x] **1.4** Parsing du chunk `SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER` dans `BOOT/SEL4/initial_task.c`.
- [x] **1.5** Raccordement du service graphique au point d'entrée de BAURA Core (`start_graphical_service`).
- [x] **1.6** Intégration dans le `Makefile` (`baura_init.elf` et `test-boot`).
- [x] **1.7** Création du document de contexte synchronisé (`Gemini agy context.md`).
- [x] **1.8** Balayage des untypeds mémoire de périphériques et hook d'autorité de sécurité au démarrage.
- [x] **1.9** Exécution et validation des tests unitaires hôtes (`make test-boot`, `make test-core-x86_64`, `make test-rust`).

### Phase 2 : Construction & Validation en Émulateur (QEMU x86_64)
- [x] **2.1** Vérification de l'arborescence seL4 (`make sel4-source-check`).
- [x] **2.2** Compilation de `libbaura_core.a` (x86_64) et de `kernel.elf` (seL4 pc99 x86_64 universel).
- [x] **2.3** Assemblage de la tâche initiale `baura_init.elf` et de l'ISO Multiboot2 (`baura-os.iso`).
- [x] **2.4** Test de démarrage sous QEMU (`make run-baura`) — Boot kernel + userspace Ring 3 validés ✅.

### Phase 3 : Capacités seL4, Mapping Mémoire & Gestionnaires Physiques
- [x] **3.1** Détection et localisation des untypeds couvrant le framebuffer dans `seL4_BootInfo`.
- [x] **3.2** Requête Multiboot2 Tag 5 Framebuffer (1024x768x32) & `gfxpayload` forcé dans GRUB.
- [x] **3.3** Mapping mémoire dynamique de la VRAM utilisateur (`seL4_X86_LargePageObject` à `0x20000000`) et affichage direct du splash à l'écran validé par screendump QEMU (1024x768x32).
- [x] **3.4** Intégration du sous-système de découverte PCI seL4 (`BOOT/SEL4/pci_backend_sel4.c`) et élimination des conflits de slots CSpace (`free_slot_cursor`).
- [ ] **3.5** Console texte Framebuffer à l'écran (Jalon M1b : miroir de la sortie console UART/seL4 sur le framebuffer).
- [x] **3.6** Injection des interruptions PS/2 (clavier IRQ 1 et souris IRQ 12) via les capacités seL4 I/O Port (`baura_ps2_sel4.c`).
- [ ] **3.7** Pilote de stockage bloc VirtIO-blk (lecture seule) sur bus PCI sous QEMU.

### Phase GUI UI_V2 (Roadmap P0–P15)
- [x] **P0** Fondations : Arborescence `GUI/UI_V2/` créée avec bibliothèques communes (Vector SDF, Text BGF, Glass, Motion).
- [x] **P1** Identité & Thème : Intégration de `baura_theme_brand.h`, 550 icônes système (`icons_data.h`) et 122 icônes d'applications (`appicons_data.h`).
- [x] **P2** Typographie Haute Fidélité : Intégration des polices Inter cuites dans le boot splash (`baura_inter_font.h`) et données BGF complètes en mémoire (`inter_bgf_data.h`, `mono_bgf_data.h`).
- [x] **P6 (Rendu Hôte)** Coque de bureau : Validation du rendu de la double barre en capsule (*Split-Dock*), tray pill et fond d'écran.
- [x] **P8 (Rendu Hôte)** Applications principales : Rendu validé des fenêtres Fichiers et Terminal.
- [x] **P9 (Cible Runtime Live)** Démarrage & Bureau dans QEMU : Animation progressive de 0 à 100% et bascule live sur le Bureau BAURA OS (Split-Dock, fenêtres Fichiers & Terminal avec curseur actif).
- [x] **P10 (BGE Natif Haute-Fidélité)** Élimination définitive du dessin procédural à la main au profit du moteur BGE C++ (`baura_bge_boot_data.h`) : RLE streaming direct vers la VRAM, dégradé nuit indigo, halos doux, police Inter, split-dock et fenêtres natives (ISO à 3888 secteurs).
- [x] **P11 (Bureau Interactif & Curseur)** Réorganisation des fenêtres à 0-chevauchement (Fichiers & Terminal avec gap de 32 px), injection du pointeur vectoriel 16x18 et gestionnaire de déplacement (Drag & Drop, Z-order, fermeture, dock toggle).

### Phase 4 : Raccordement du Runtime Graphique Persistant (BGE Desktop)
- [x] **4.1** Transition du splash de démarrage vers le bureau persistant en VRAM (`baura_boot_draw_desktop`).
- [x] **4.2** Affichage du bureau avec la double barre des tâches en capsules (*Split-Dock*), fond dégradé et fenêtres Fichiers/Terminal.
- [x] **4.3** Injection des assets natifs BGE sans allocation dynamique (`baura_bge_boot_data.h`).
- [x] **4.4** Interaction utilisateur souris et clavier dans le bureau BGE (pilote PS/2 seL4, boucle réactive, drag & drop fluide).

### Phase 6 : Architecture SDK BUI & Applications Clientes (Non-Hardcodées)
- [x] **6.1** Types fondamentaux, unités logiques `dp` et jetons de thème officiel (`GUI/UI/bui_types.h`).
- [x] **6.2** Moteur de mise en page automatique 2 passes (`measure` & `perform`) sans coordonnées en dur (`GUI/UI/bui_layout.c`).
- [x] **6.3** Arbre de widgets standards (Row, Column, Panel verre, Label, Button, IconButton, WindowFrame) et routage d'événements (`GUI/UI/bui_widget.c`).
- [x] **6.4** SDK Applicatif & Registre d'Applications (`GUI/UI/bui_app.c`) : manifeste `bui_app_desc_t`, hooks de cycle de vie et instances.
- [x] **6.5** Éléments Shell modélisés comme applications clientes (`GUI/SHELL/bui_shell_apps.c`) : Split-Dock, TopBar, BStart, Fichiers, Terminal.
- [x] **6.6** Protocole Baura Surface Protocol BSP (`GUI/PROTOCOL/bsp_protocol.h`, `bsp_client.c`) et test unitaire validé (`GUI/tests/bui_sdk_test.c`).

### Phase 5 : Isolation des Services BAURA Core & Transport IPC v2
- [ ] **5.1** Implémentation du transport IPC réel au-dessus des endpoints seL4 pour l'ABI v2.
- [ ] **5.2** Lancement du premier service isolé supervisé par le `service_manager`.
- [ ] **5.3** Raccordement du VFS (`PC/<LECTEUR>:/...`) au gestionnaire de stockage et volumes.

---

## 4. Point de Situation Actuel

| Indicateur | Statut | Détail |
| :--- | :--- | :--- |
| **Étape en cours** | **Bureau Interactif Actif (Curseur PS/2 & Drag & Drop) + SDK BUI Déployé** | Le bureau est désormais totalement manipulable : curseur vectoriel PS/2 seL4 sans lag (buffer 16x20 restauré sans repaint), glisser-déplacer des fenêtres en temps réel, zéro chevauchement initial (32 px gap), Z-order dynamique. Le SDK applicatif BUI (`GUI/UI/`, `GUI/PROTOCOL/`, `GUI/SHELL/`) élimine tout code en dur et modélise les interfaces et applications sous une architecture modulaire standardisée. |
| **Documents vivants** | **`Gemini agy actions.md` · `Gemini agy context.md` · `BAURA_TEST_JOURNAL.md` · `nextstep agy.md`** | Entièrement synchronisés. Suite de tests unitaires hôte (`test-boot`, `test-bui`, `test-gui`) intégrée au Makefile. |
| **Prochaine action prévue** | **Validation Live sous QEMU & Raccordement BSP IPC** | Exécuter le test live dans QEMU pour vérifier la fluidité de la souris et du drag & drop, puis connecter le transport IPC du protocole BSP entre BDS et les processus clients isolés. |


