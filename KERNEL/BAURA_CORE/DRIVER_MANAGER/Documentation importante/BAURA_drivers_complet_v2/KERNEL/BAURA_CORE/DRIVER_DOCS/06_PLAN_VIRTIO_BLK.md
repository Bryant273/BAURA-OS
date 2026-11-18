# 06 — Plan du premier pilote réel : virtio-blk (lecture seule)

Pourquoi virtio-blk d'abord : spécification publique, pas de matériel
propriétaire, reproductible sous QEMU, et il exerce tout le chemin
(découverte PCI → ressources → DMA → IRQ → volume).

Constantes et structures : [`examples/virtio_pci.h`](./examples/virtio_pci.h)
(tailles vérifiées par `_Static_assert`, compilé).

## 1. État de vérification des faits

**[vérifié]** (en-têtes Linux/U-Boot, spécification virtio) : vendor `0x1AF4` ;
IDs modernes `0x1040–0x107F`, type = id − `0x1040` ; bits d'état ACKNOWLEDGE 1,
DRIVER 2, DRIVER_OK 4, FEATURES_OK 8, NEEDS_RESET 64, FAILED 128 ; capacités
PCI de vendeur (id `0x09`) types COMMON 1, NOTIFY 2, ISR 3, DEVICE 4, PCI_CFG 5 ;
disposition de `virtio_pci_common_cfg` ; VIRTIO_F_VERSION_1 = bit 32 ; virtqueue
(desc 16 octets `{addr,len,flags,next}`, flags NEXT 1/WRITE 2/INDIRECT 4,
alignements 16/2/4) ; bloc : requêtes IN 0, OUT 1, FLUSH 4, statuts OK 0,
IOERR 1, UNSUPP 2, `outhdr {u32 type, u32 ioprio, u64 sector}`, capacité en
secteurs de 512 octets.

**[à confirmer]** — la spécification complète n'a pas pu être lue (la page a été
tronquée) : le type de périphérique bloc = 2 donc ID moderne `0x1042` ; la plage
transitoire `0x1000–0x103F` et le bloc `0x1001` ; l'**ordre exact** d'initialisation
(reset → ACKNOWLEDGE → DRIVER → features → FEATURES_OK puis **relire** l'état →
files → DRIVER_OK) ; l'option QEMU `disable-legacy=on`. À relire dans la
spécification OASIS virtio 1.x avant de coder.

## 2. Étapes

1. **Découverte** : parcourir le PCI, repérer `0x1AF4` + ID dans `0x1040–0x107F`
   (bloc) ; lire la liste de capabilities (déjà parcourue, bornée) pour les
   cfg COMMON/NOTIFY/ISR/DEVICE ; chaque cap donne `bar`, `offset`, `length`.
2. **Ressources** (nécessite BAR sizing + mapping : non implémentés, guide M3) :
   mapper le BAR référencé (attribut `CacheDisabled`).
3. **Initialisation** (ordre à confirmer, §1) ; refuser si VERSION_1 absent.
4. **File 0** : sélectionner, lire `queue_size` (puissance de 2, non nul),
   allouer desc/avail/used contigus et alignés (`dma_alloc`), écrire les adresses
   **bus**, `queue_enable = 1`.
5. **Requête** : 3 descripteurs chaînés : en-tête (lecture device), données
   (WRITE pour une lecture disque), statut 1 octet (WRITE) ; publier dans
   `avail`, notifier à `notify_base + queue_notify_off × multiplier`.
6. **Attente** : d'abord par **scrutation** de `used.idx` avec délai borné ; IRQ
   (ISR/MSI) ensuite.
7. **Volume** : `BauraBlockDevice{block_count = capacité, block_size = 512 ou
   blk_size, read, write = refus}` ; nécessite le drapeau lecture seule (`04_…`).

## 3. Test sous QEMU — [à confirmer à l'exécution]

```sh
dd if=/dev/zero of=disk.img bs=1M count=8
printf 'BAURA-MARKER' | dd of=disk.img bs=512 seek=7 conv=notrunc
qemu-system-x86_64 -machine q35 -m 512 -cdrom <image>.iso -serial stdio \
  -drive if=none,id=d0,file=disk.img,format=raw,readonly=on \
  -device virtio-blk-pci,drive=d0,disable-legacy=on
```

Critère : lecture du secteur 7 renvoie `BAURA-MARKER` ; écriture refusée ;
capacité = 16384 secteurs ; secteur hors limites refusé.

## 4. Tests sur hôte (avant QEMU)

Faux périphérique virtio simulant COMMON cfg + file : tests de l'ordre
d'initialisation, d'un périphérique qui ne met jamais `FEATURES_OK`, d'une file
de taille 0 ou non puissance de 2, d'un `used.idx` qui ne bouge pas (délai), d'un
statut IOERR. Aucun de ces tests n'existe encore.
