# 02 — Lire le BootInfo et afficher une console framebuffer (jalon M1b)

Code d'exemple compilé et testé sur hôte :
[`examples/bootinfo_chunks.c`](./examples/bootinfo_chunks.c). Il lit un tampon
d'octets ; la liaison au vrai `seL4_BootInfo` reste à écrire et **n'a pas été
compilée contre les en-têtes générés**.

## 1. Disposition du BootInfo (x86_64) — [vérifié dans les sources épinglées]

- Page principale `seL4_BootInfo`, puis `extraLen` octets de chunks à partir de
  l'offset `seL4_BootInfoFrameSize` (4096).
- Chaque chunk commence par `seL4_BootInfoHeader { seL4_Word id; seL4_Word len; }`.
  **`len` inclut l'en-tête.** Les chunks sont **empaquetés** : un en-tête peut
  être **non aligné** → lire avec `memcpy` (comme dans l'exemple).

| id | Nom | Contenu |
| --- | --- | --- |
| 0 | PADDING | remplissage |
| 1 | X86_VBE | seulement si `vbeMode != -1` |
| 2 | X86_MBMMAP | en-tête + `u32 mmap_length` (octets) + entrées packed `{u32 size, u64 base_addr, u64 length, u32 type}` (type 1 = RAM) |
| 3 | X86_ACPI_RSDP | **copie** de `acpi_rsdp_t` (signature, checksum, oem_id, revision, rsdt_address, length, xsdt_address, …) |
| 4 | X86_FRAMEBUFFER | `multiboot2_fb_t` packed : `u64 addr, u32 pitch, u32 width, u32 height, u8 bpp, u8 type` |
| 5 | X86_TSC_FREQ | en-tête + `u32` en MHz |
| 6 | FDT | arbre de périphériques |

libsel4 ne définit que `seL4_X86_BootInfo_fb_t` comme alias de `struct
multiboot2_fb`, dont la définition est dans un en-tête **interne au noyau** :
l'espace utilisateur doit définir son **propre miroir** (`BauraBootFb`). Le
`_Static_assert`/le test vérifient 22 octets.

**Règle de sécurité :** toute lecture valide `len >= 16`, `len <= restant`, la
taille de la charge utile attendue, et traite le contenu comme non fiable.

## 2. Le framebuffer n'existe pas toujours — [vérifié]

Le chunk 4 n'est émis que si le bootloader fournit une étiquette framebuffer
Multiboot2. L'en-tête Multiboot2 de seL4 ne demande aucun mode ; GRUB peut
quand même fournir un framebuffer selon sa configuration (`gfxpayload`)
**[à confirmer par essai]**. Le noyau imprime « Got framebuffer info in
multiboot2 … » quand il l'a reçu : **c'est le premier test à faire sous QEMU**.
Le champ `type` : 0 indexé, 1 RGB direct, 2 texte EGA (adresse 0xB8000)
**[à confirmer dans la spécification Multiboot2 avant d'écrire le code]**.
Le code doit refuser tout `type`/`bpp` non géré (ex. n'accepter que type 1,
bpp 32 au début) et retomber sur le port série.

## 3. Mémoire utilisable — [vérifié]

Les untypeds sont listés dans `untypedList` (`paddr`, `sizeBits`, `isDevice`).
Les untypeds *device* couvrent les trous non-RAM de l'espace physique jusqu'à
`CONFIG_PADDR_USER_DEVICE_TOP`. Le framebuffer et les BAR PCI sont donc des
untypeds **device** : on les `Retype` en frames.

## 4. Mapper le framebuffer — [proposition, API vérifiée]

1. Trouver l'untyped device contenant `fb.addr` (parcourir `untypedList` avec
   `isDevice`). Si l'untyped est plus grand que nécessaire, le `Retype` en
   frames 4 Ko consécutives **à partir de la bonne adresse** impose de
   découper : retyper d'abord en objets plus petits jusqu'à atteindre l'adresse
   voulue (les objets d'un untyped sont alloués dans l'ordre).
2. `seL4_Untyped_Retype(ut, seL4_X86_4K, 0, root, node_index, depth, slot, n)`
   pour obtenir les frames (profondeur `seL4_WordBits` pour la racine).
3. Mapper : `seL4_X86_Page_Map(frame, vspace, vaddr, rights, attr)`. Si la
   table de pages manque, `Map` échoue : créer `seL4_X86_PageTableObject` (et
   éventuellement PD/PDPT) puis `…_Map` et réessayer.
4. Attributs : `seL4_X86_Default_VMAttributes` = 0, `seL4_X86_CacheDisabled` = 2,
   `seL4_X86_WriteCombining` = 4. **[proposition]** commencer par `Default`
   (correct, lent), puis tester WriteCombining ; sur machine réelle, un mauvais
   attribut se traduit par lenteur ou artefacts, pas par panne.
5. Valider `pitch >= width * bpp/8` et `height * pitch` ne déborde pas.

## 5. Console texte — [proposition]

Police bitmap 8×16 embarquée, rendu pixel par pixel dans le framebuffer
(bpp 32 d'abord), défilement par `memmove` de lignes, double sortie : toute
ligne envoyée à `seL4_DebugPutChar` est aussi dessinée. Le rendu GUI existant
(`GUI/`) fournit déjà un moteur logiciel qu'on peut réutiliser ; à décider.

## 6. Critères d'acceptation M1b

- Sous QEMU : le texte de la liste PCI est visible à l'écran **et** sur le
  série, identique.
- Test hôte : chunks tronqués, `len` incohérent, chunk absent (déjà dans
  `examples/examples_test.c`), et `pitch` invalide → pas de crash, repli série.
- Sur PC réel : mêmes lignes qu'avec QEMU pour le format, contenu = `lspci -nn`.
