# 06 — Écran de démarrage et framebuffer UEFI / seL4

| Code | Rendu |
|---|---|
| `code/baura_boot_splash.h` — séquence animée, fonction pure du temps | `render/splash-sequence.png` (6 instants : 0,05 → 2,2 s) · `render/splash-final.png` (réduction de mouvement) |
| `code/baura_mb2_fb.h` — lecture de l'étiquette framebuffer Multiboot2 + `present_argb` avec pitch | — |
| `code/test_boot.cpp` — 13 vérifications (blob synthétique, cas refusés, pitch, RGB/BGR) | `./test_boot` → « 06 : tous les tests passent » |

![séquence](render/splash-sequence.png)

## 1. Constat dans le dépôt
- `BOOT/README.md` : l'écran de démarrage n'est **pas affiché au vrai boot** (chemin seL4 sans framebuffer). `draw_boot_splash` n'existe que dans le rendu d'aperçu (dégradé + panneau + marque en pixels + « BAURA OS » en 5×7).
- `BOOT/SEL4/grub.cfg` charge `kernel.elf` en `multiboot2` et `baura_init.elf` en `module2` : **aucune demande de mode graphique**.
- `GUI/PLATFORM` sait déjà présenter dans un `BauraMappedFramebuffer{pixels, capacity_bytes, width, height, pitch_bytes, format, flush}` (RGBX8888 / BGRX8888). Il manque donc uniquement : **obtenir** ce framebuffer au boot, et **dessiner** un splash correct.

## 2. Séquence du splash (≈ 2,4 s)
| t (s) | Élément | Courbe |
|---|---|---|
| 0 – 0,25 | fondu du fond + halo (5 disques à alpha ≈ 3,5 %, pas de flou : coût nul) | ease-out |
| 0,10 – 0,55 | la barre indigo pousse depuis son centre | ease-out |
| 0,45 – 0,95 | les deux demi-disques (violet / lavande) sont dévoilés par un clip qui avance | ease-out |
| 0,85 – 1,25 | le point doré grossit en ressort (dépasse 1 puis se pose) | `SPRING` |
| 1,10 – 1,60 | « BAURA OS » (Inter SemiBold 28), barre de progression 220×6, ligne d'état (Inter 14) | ease-out |

Règles : la progression affichée est la **progression réelle** (jalons du boot : noyau, serveur de fichiers, affichage, entrée…), jamais un faux compteur. En `reduce_motion`, on dessine directement l'état final. L'image dépend uniquement de `(t, progress, status)` : aucun état à synchroniser, rendu reproductible en test.

## 3. Framebuffer : du bootloader au pixel
### 3.1 GRUB (Multiboot2) — vérifié sur la spécification, à tester sur QEMU
```
# grub.cfg — ajouter AVANT multiboot2
set gfxmode=1280x720x32,1024x768x32,auto
set gfxpayload=keep          # garde le mode choisi par GRUB pour le noyau
multiboot2 /boot/kernel.elf
```
et dans l'en-tête Multiboot2 du noyau, l'étiquette « framebuffer » (type 5) : `width=1280, height=720, depth=32` (préférence, le firmware peut en donner un autre). Le noyau reçoit alors l'étiquette **type 8** analysée par `find_framebuffer()` :

| Offset | Champ | Utilisation |
|---|---|---|
| 0 | `addr` u64 | adresse **physique** à mapper |
| 8 | `pitch` u32 | **octets par ligne**, souvent > largeur×4 |
| 12/16 | `width`, `height` u32 | |
| 20 | `bpp` u8 | 32 exigé |
| 21 | `type` u8 | **1 = RGB direct** exigé (0 = palette, 2 = texte EGA : refusés) |
| 24… | `red_pos/size`, `green_pos/size`, `blue_pos/size` | `red_pos=16` ⇒ octets B,G,R,X ⇒ `BGRX8888` ; `red_pos=0` ⇒ `RGBX8888` |

Le test couvre : blob valide BGR et RGB, palette, texte, 16 bpp, blob tronqué, taille d'étiquette corrompue, pointeur nul, adresse > 4 Gio (`0xFD00000000`) et pitch avec bourrage.

### 3.2 Correspondance avec le dépôt
```cpp
BauraBoot::FbInfo f;
if (!BauraBoot::find_framebuffer(mb2_info, mb2_max, f)) { /* repli : console série, pas de GUI */ }
BauraMappedFramebuffer fb{ (volatile uint8_t*)mapped_vaddr, f.bytes(), f.width, f.height, f.pitch_bytes,
                           f.is_bgrx() ? BauraFramebufferFormat::BGRX8888 : BauraFramebufferFormat::RGBX8888,
                           flush_cb, nullptr };
platform.initialize(fb);     // GUI/PLATFORM/baura_gui_platform.cpp (inchangé)
```

### 3.3 seL4 : donner la mémoire vidéo à la tâche GUI — **non compilé, à confirmer**
Le sous-module seL4 n'est pas extrait dans le dépôt cloné ; les noms ci-dessous sont ceux de l'API seL4 documentée, à **vérifier** contre la version épinglée :
1. Le noyau expose les régions de périphérique dans `seL4_BootInfo` (`untyped` avec `isDevice=1`, adresse/taille dans `untypedList`). **À confirmer** : sur x86_64, le framebuffer GRUB apparaît-il comme untyped de périphérique ? (sinon, il faut qu'il soit déclaré dans la mémoire « device » par l'amorce, ou accessible via `seL4_X86_BootInfo_VBE` — champ historique à vérifier.)
2. Trouver l'untyped dont `[paddr, paddr+size)` contient `f.phys_addr`.
3. `seL4_Untyped_Retype(..., seL4_X86_4K, ...)` pour chaque page de 4 Ko (ou `seL4_X86_LargePageObject` si aligné sur 2 Mio), puis `seL4_X86_Page_Map(page, vspace, vaddr, seL4_ReadWrite, seL4_X86_Default_VMAttributes)`.
4. **Attributs de cache** : demander *write-combining* (PAT) si disponible ; sinon le non-cacheable par défaut fonctionne mais les écritures octet par octet sont lentes → d'où le tampon `staging` ARGB déjà présent dans `BauraGuiPlatform` (écrire par rafales de lignes, jamais pixel par pixel).
5. Vérifier `capacity_bytes ≥ pitch × height` avant d'initialiser.

### 3.4 UEFI (GOP) — alternative sans GRUB graphique
`EFI_GRAPHICS_OUTPUT_PROTOCOL` fournit `FrameBufferBase`, `PixelsPerScanLine` (×4 = pitch) et `PixelFormat` (`PixelRedGreenBlueReserved8BitPerColor` ⇒ RGBX, `PixelBlueGreenRedReserved8BitPerColor` ⇒ BGRX). Même `FbInfo`, autre source ; garder le parseur Multiboot2 comme chemin par défaut tant que GRUB reste le chargeur.

## 4. Passer de l'état actuel à la cible
| Étape | Fichier du dépôt | Changement |
|---|---|---|
| 1 | `BOOT/SEL4/grub.cfg` | `gfxmode` + `gfxpayload=keep` |
| 2 | `BOOT/SEL4/…` (en-tête Multiboot2 du noyau / amorce) | étiquette framebuffer ; transmettre l'adresse de l'info à `initial_task.c` |
| 3 | `BOOT/SEL4/initial_task.c` | `find_framebuffer`, mapper les pages (§3.3), remplir `BauraMappedFramebuffer` |
| 4 | `GUI/…/draw_boot_splash` | remplacer par `BauraBoot::draw_splash` ; appeler à chaque jalon de boot |
| 5 | `Makefile` | `test-gui` : ajouter `test_boot` ; QEMU : `-device virtio-vga` ou `-vga std` + capture `screendump` pour un test de non-régression |
| 6 | `BOOT/README.md` | retirer la mention « pas de framebuffer » une fois testé |

**Critères d'acceptation** : (a) sur QEMU 1280×720, l'image du splash s'affiche sans décalage de lignes (pitch correct) ; (b) couleurs correctes en RGB et BGR ; (c) boot sans framebuffer ⇒ pas de plantage, repli série ; (d) le splash n'allonge pas le boot de plus de 50 ms hors animation.

## 5. Limites honnêtes
- Rendu et parseur **testés sur hôte** ; la partie seL4 (§3.3) est de la **conception**, pas du code compilé.
- Le halo montre un léger étagement (5 anneaux) : à lisser avec un dégradé radial (à ajouter à `baura_vector.h`) ; il sera invisible à 8 bits dans la plupart des cas.
- Pas de son ni de gestion de plusieurs écrans.
