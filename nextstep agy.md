# Guide d'Analyse & Prochaines Étapes (Next Steps) — BAURA OS

> **Document de pilotage & résolution des erreurs de build / boot**  
> **Auteur :** Antigravity (Gemini) pour Ahmed (SDA)  
> **Date :** 16 novembre 2026 (Intégration du véritable rendu natif BGE haute fidélité)  
> **Cible :** x86_64 pc99 · seL4 16.0.0-dev · BAURA Core · BGE GUI / UI_V2

---

## 1. Analyse Technique Approfondie des Dernières Erreurs

### A. L'erreur `non-thread-local declaration of '__sel4_ipc_buffer' follows thread-local declaration`

#### Contexte & Symptôme
Lors de la compilation de la tâche initiale (`baura_init.o`) :
```text
BOOT/SEL4/initial_task.c:20:17: error: non-thread-local declaration of ‘__sel4_ipc_buffer’ follows thread-local declaration
   20 | seL4_IPCBuffer *__sel4_ipc_buffer = 0;
      |                 ^~~~~~~~~~~~~~~~~
In file included from build/sel4-x86_64/stage/libsel4/include/sel4/functions.h:13:45:
   13 | extern LIBSEL4_THREAD_LOCAL seL4_IPCBuffer *__sel4_ipc_buffer;
```

#### Cause Racine
1. Dans `libsel4/include/sel4/functions.h`, seL4 déclare le pointeur vers le tampon IPC avec la macro `LIBSEL4_THREAD_LOCAL`.
2. Par défaut, la configuration CMake de `libsel4` active l'option `LibSel4UseThreadLocals=ON`, ce qui fait que `LIBSEL4_THREAD_LOCAL` se développe en `__thread` (Thread Local Storage).
3. Notre déclaration manuelle dans `initial_task.c` était `seL4_IPCBuffer *__sel4_ipc_buffer = 0;` (sans `__thread`), ce qui crée un conflit de spécificateur de stockage pour GCC.
4. **Problème d'architecture sous-jacent :** Dans un binaire racine autonome (*freestanding* sans libc) comme `baura_init.elf`, aucun runtime TLS n'est encore initialisé (le registre de segment `%fs` ou les MSRs FS_BASE ne pointent vers aucun bloc TCB utilisateur). Utiliser `__thread` force le compilateur à générer des accès relatifs à `%fs:offset`, ce qui provoquerait une faute générale de protection (`#GP`) à l'exécution dès le premier accès au tampon IPC !

#### Solution Appliquée
1. **Dans le code source (`BOOT/SEL4/initial_task.c`) :**
   Définition avec la macro native `LIBSEL4_THREAD_LOCAL` :
   ```c
   LIBSEL4_THREAD_LOCAL seL4_IPCBuffer *__sel4_ipc_buffer = 0;
   #ifdef CONFIG_KERNEL_INVOCATION_REPORT_ERROR_IPC
   LIBSEL4_THREAD_LOCAL char __sel4_print_error = 0;
   #endif
   ```
2. **Dans le linker script (`BOOT/SEL4/initial_task.ld`) :**
   Intégration des sections `.tdata` et `.tbss` dans les segments `.data` et `.bss` pour garantir l'édition de liens quel que soit le mode de compilation.
3. **Dans la configuration CMake du noyau (`Makefile`) :**
   Ajout de `-DLibSel4UseThreadLocals=OFF` dans la cible `sel4-configure-x86_64` :
   ```makefile
   sel4-configure-x86_64: sel4-source-check
   	$(HOST_CMAKE) -S $(SEL4_SOURCE_DIR) -B $(SEL4_X86_64_BUILD_DIR) \
   		-DKernelPlatform=pc99 -DKernelSel4Arch=x86_64 \
   		-DKernelVerificationBuild=OFF -DKernelPrinting=ON \
   		-DKernelSupportPCID=OFF -DKernelHugePage=OFF -DKernelFSGSBase=msr \
   		-DLibSel4UseThreadLocals=OFF \
   		$(SEL4_CMAKE_ARGS)
   ```
   *Bénéfice :* Garantit que `__sel4_ipc_buffer` est un pointeur global classique, évitant tout piège TLS à l'exécution.

---

### B. Le problème du Framebuffer EGA 80x25 vs Rendu Graphique 1024x768

#### Contexte & Symptôme
Le noyau seL4 bootait, la console série affichait `"BAURA OS Core ready"`, mais l'écran QEMU restait figé sur le message BIOS texte :
```text
Got framebuffer info in multiboot2. Current video mode is at physical address=b8000 pitch=160 resolution=80x25@16 type=2
BAURA_INIT: Framebuffer detected: 80x25@16 at 0x00000000000B8000 (pitch=160 type=2)
```

#### Cause Racine
* L'en-tête Multiboot2 de seL4 (`multiboot.S`) ne contenait aucun tag de demande de mode vidéo.
* GRUB conservait son mode console par défaut (mode texte 80x25 EGA à l'adresse physique `0xB8000`).
* La fonction `baura_boot_framebuffer_validate` rejette tout mode différent de 32 bpp linéaire (RGBX/BGRX).

#### Solution Appliquée
1. **Tag Multiboot2 Framebuffer (Tag 5)** injecté dans `KERNEL/L4/sel4/src/arch/x86/multiboot.S` :
   - Type = 5 (`MULTIBOOT_HEADER_TAG_FRAMEBUFFER`)
   - Flags = 1 (Optionnel)
   - Largeur = 1024 px, Hauteur = 768 px, Profondeur = 32 bpp
2. **GRUB Video Payload forcé** dans `BOOT/SEL4/grub.cfg` :
   - `set gfxpayload=1024x768x32,1024x768,auto`
   - Chargement des modules `all_video`, `vbe`, `vga`, `video_bochs`, `video_cirrus`.
3. **Mappeur VRAM Userspace (`map_framebuffer`)** dans `BOOT/SEL4/initial_task.c` :
   - Retypage de l'untyped périphérique en pages 2 Mo (`seL4_X86_LargePageObject`).
   - Mapping dans le VSpace à l'adresse virtuelle `0x20000000`.
   - Connexion de `g_boot_fb.pixels = (void *)0x20000000`.

---

## 2. Procédure de Synchronisation & Démarrage QEMU (UI_V2 & Boot Splash Inter)

### Commande unique Tout-en-Un (Copier-Coller dans WSL)
Exécutez cette commande dans votre terminal WSL Ubuntu (`ahmed@PROBOOK-OF-SIL-DOM-MEDOOO:~/BAURA-OS`) pour synchroniser tous les composants modifiés (y compris la police Inter et les bibliothèques UI_V2) et démarrer l'OS dans QEMU :

```bash
cd ~/BAURA-OS && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/Makefile ~/BAURA-OS/Makefile && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/grub.cfg ~/BAURA-OS/BOOT/SEL4/grub.cfg && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.c ~/BAURA-OS/BOOT/SEL4/initial_task.c && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.ld ~/BAURA-OS/BOOT/SEL4/initial_task.ld && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/pci_backend* ~/BAURA-OS/BOOT/SEL4/ && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/pci_bar* ~/BAURA-OS/BOOT/SEL4/ && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/* ~/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/ && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/tests/pci_* ~/BAURA-OS/BOOT/tests/ && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/KERNEL/BAURA_CORE/DEVICE_MANAGER/BAURA_DEVICE_MGR* ~/BAURA-OS/KERNEL/BAURA_CORE/DEVICE_MANAGER/ && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/KERNEL/BAURA_CORE/DEVICE_MANAGER/baura_device_report* ~/BAURA-OS/KERNEL/BAURA_CORE/DEVICE_MANAGER/ && \
rm -f ~/BAURA-OS/KERNEL/BAURA_CORE/DEVICE_MANAGER/pci_scanner* && \
mkdir -p ~/BAURA-OS/KERNEL/BAURA_CORE/HARDWARE_MANAGER ~/BAURA-OS/KERNEL/BAURA_CORE/DRIVER_DOCS ~/BAURA-OS/GUI/UI_V2 && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/KERNEL/BAURA_CORE/HARDWARE_MANAGER/* ~/BAURA-OS/KERNEL/BAURA_CORE/HARDWARE_MANAGER/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/KERNEL/BAURA_CORE/DRIVER_DOCS/* ~/BAURA-OS/KERNEL/BAURA_CORE/DRIVER_DOCS/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/GUI/UI_V2/* ~/BAURA-OS/GUI/UI_V2/ && \
make baura-iso && \
qemu-system-x86_64 -m 2G -cdrom build/baura-os.iso -cpu max -vga std -serial stdio
```

---

## 3. Ce qui a été validé avec succès (Rendu Haute Fidélité BGE & Fin du Dessin Procédural)

1. **Fin du Dessin « à la main » & Conformité au Cahier des Charges :**
   - Remplacement des tracés procéduraux hardcodés par le véritable moteur vectoriel BGE C++ (`baura_vector.h`, `baura_text.h`, `baura_glass.h`, `baura_theme_brand.h`, `baura_ui.h`, `baura_shell.h`).
   - Compilation et validation du bureau natif complet sous hôte (`render_full_desktop.cpp`) générant [`bge_real_desktop.png`](file:///C:/Users/SDA/Downloads/bge_real_desktop.png) et [`bge_real_desktop.bmp`](file:///C:/Users/SDA/Downloads/bge_real_desktop.bmp).
   - Validation du splash animé vectoriel en ressorts cubiques (`test_boot.cpp` -> `splash_final.png` et `splash_strip.png`).

2. **Injection sans Allocation Dynamique dans l'Initial Task seL4 :**
   - Encodage RLE sans perte des scènes BGE via `generate_bge_boot_assets.py`.
   - Création de [`baura_bge_boot_data.h`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_bge_boot_data.h) (116 569 runs pour le bureau, 22 894 runs pour le splash).
   - Streaming direct vers le framebuffer VRAM utilisateur (`0x20000000`) dans [`baura_boot_splash.c`](file:///c:/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.c).
   - **ISO compilée avec succès :** 3888 secteurs, 0 erreur, 0 avertissement avec `-Wall -Wextra -Werror`.

3. **Bascule Live vers le Bureau Officiel BAURA OS :**
   - **Split-Dock** en bas avec bouton Démarrer vectoriel, champ de recherche et lanceurs.
   - **Top-Tray** en haut à droite avec température, indicateurs et notifications.
   - Fenêtre **Fichiers** native (sidebar, breadcrumbs, grille de dossiers zébrée).
   - Fenêtre **Terminal** native avec typographie monospace et invite colorée.

---

## 4. Organisation du Travail sur GUI/, BOOT/, KERNEL/ et DRIVERS

### 1. Démarrage Rapide & Test dans QEMU (Commande Utilisateur)
Pour copier l'ensemble des fichiers mis à jour dans votre environnement WSL, compiler la suite de tests et lancer l'OS interactif avec souris, Drag & Drop et validation du SDK .AESB :
```bash
cd ~/BAURA-OS && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/baura_ps2_sel4.* ~/BAURA-OS/BOOT/SEL4/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/SEL4/initial_task.c ~/BAURA-OS/BOOT/SEL4/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_desktop_manager.* ~/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_boot_splash.* ~/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/tests/desktop_manager_test.c ~/BAURA-OS/BOOT/tests/ && \
mkdir -p ~/BAURA-OS/GUI/UI ~/BAURA-OS/GUI/PROTOCOL ~/BAURA-OS/GUI/SHELL && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/GUI/UI/* ~/BAURA-OS/GUI/UI/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/GUI/PROTOCOL/* ~/BAURA-OS/GUI/PROTOCOL/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/GUI/SHELL/* ~/BAURA-OS/GUI/SHELL/ && \
cp -r /mnt/c/Users/SDA/Downloads/BAURA-OS/GUI/tests/bui_sdk_test.c ~/BAURA-OS/GUI/tests/ && \
cp /mnt/c/Users/SDA/Downloads/BAURA-OS/Makefile ~/BAURA-OS/Makefile && \
make test-boot && make test-bui && make baura-iso && \
qemu-system-x86_64 -m 2G -cdrom build/baura-os.iso -cpu max -vga std -serial stdio
```

### 2. Ce qu'il faut observer et manipuler dans QEMU :
1. **Zéro chevauchement :** Les fenêtres **Fichiers** (gauche) et **Terminal** (droite) apparaissent parfaitement espacées (32 px d'écart) sans se recouvrir.
2. **Curseur de souris fluide :** Le pointeur vectoriel suit fidèlement les mouvements de votre souris dans QEMU.
3. **Glisser-Déplacer (Drag & Drop) :** Cliquez et maintenez le bouton gauche sur la barre de titre d'une fenêtre pour la déplacer librement sur tout le bureau.
4. **Ordre Z et Focus :** Cliquez sur une fenêtre pour la faire passer devant l'autre.
5. **Bouton Fermer (Rouge) :** Cliquez sur la pastille rouge en haut à gauche d'une fenêtre pour la masquer.
6. **Split-Dock & BStart :** Cliquez sur l'icône de l'application dans la capsule inférieure pour restaurer sa fenêtre, et cliquez sur le logo BAURA pour ouvrir/fermer le menu Démarrer.

### 3. Architecture Logicielle, SDK BUI & Format Exécutable .AESB
- **Format Officiel `.aesb` (Appli Exécutable Sur BAURA) :** Standardisé dans [`AESB_SPEC.md`](file:///c:/Users/SDA/Downloads/BAURA-OS/INFRASTRUCTURE/SPECIFICATIONS/SYSTEM_SPECIFICATIONS/AESB_SPEC.md) et implémenté dans `GUI/UI/baura_aesb.c`. Contient le manifeste BUI, le code Ring 3 seL4, les permissions de sécurité et les ressources.
- **Fin des interfaces figées ou codées en dur :** Tout composant est structuré autour du SDK BUI (`GUI/UI/bui_app.h`), avec moteur de layout en 2 passes (`measure` puis `perform`), unités logiques en `dp` et protocole de surface BSP (`GUI/PROTOCOL/bsp_protocol.h`).
- **Composants d'interface = Applications clientes :** Le Dock, la TopBar, le menu BStart, ainsi que Fichiers et Terminal sont tous définis selon le même cycle de vie applicatif (`bui_app_desc_t`).

### 2. Aperçu Direct des Rendus Graphiques sur Windows
Les images haute définition générées par le moteur BGE sont directement accessibles sur votre machine :
* Bureau BGE Natif : [`C:\Users\SDA\Downloads\bge_real_desktop.png`](file:///C:/Users/SDA/Downloads/bge_real_desktop.png)
* Séquence Animée Splash : [`C:\Users\SDA\Downloads\splash_strip.png`](file:///C:/Users/SDA/Downloads/splash_strip.png)
* Splash Final : [`C:\Users\SDA\Downloads\splash_final.png`](file:///C:/Users/SDA/Downloads/splash_final.png)

### 3. Interaction Matérielle PS/2 (Jalon Suivant)
- Acheminer les interruptions clavier IRQ 1 et souris IRQ 12 vers le thread utilisateur via `seL4_IRQControl_Get`.
- Raccorder les événements souris au déplacement du curseur sur le bureau.

### 4. KERNEL (seL4 + BAURA Core)
- Allocateur mémoire utilisateur (`MEM-0001`) exploitant les untypeds RAM libres du `BootInfo`.
- Transport IPC v2 sur endpoints seL4 pour découpler les services en Ring 3.
- Superviseur de services (`service_manager`) démarrant les processus isolés.

