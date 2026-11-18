# BAURA OS — Guide des pilotes : développement, listing, attribution, mise à jour

Ce guide décrit comment faire de BAURA OS un système capable de **détecter la
majorité des matériels x86_64, d'attribuer le bon pilote et de le mettre à
jour**, en commençant par des pilotes open source.

Des documents détaillés complètent ce guide dans [`DRIVER_DOCS/`](./DRIVER_DOCS/README.md) : démarrage PC réel, BootInfo/framebuffer, concurrence, interfaces de classe, sécurité, virtio-blk, paquets/mises à jour, décisions ouvertes.

Il complète [`DEVICE_DRIVER_STATUS.md`](./DEVICE_DRIVER_STATUS.md), qui est la
référence sur **ce qui existe et son niveau de vérification**. Les structures et
API marquées **(proposition)** n'existent pas encore dans le dépôt.

---

## 1. Objectif et point de départ

### 1.1 Ce qui existe

- Le registre de périphériques (`DEVICE_MANAGER/BAURA_DEVICE_MGR.c`) parcourt
  le PCI via un lecteur injecté, remplit un `BauraDevice` par fonction, le
  classe, et appelle les `probe()` enregistrés.
- Un **backend PCI pour l'initial task seL4 x86_64** (`BOOT/SEL4/pci_backend.c`
  et `pci_backend_sel4.c`) fournit ce lecteur : il obtient une capability
  d'I/O pour les ports `0xCF8–0xCFF` puis installe le lecteur. L'initial task
  l'appelle après le démarrage du Core et affiche la liste des appareils.
  Il est testé sur hôte avec un faux hôte PCI ; il n'a pas encore été compilé
  avec les en-têtes seL4 générés ni démarré (voir le statut).
- Les pilotes s'enregistrent par type ou par couple exact vendor/device. Le
  `probe()` s'exécute **dans le même espace d'adressage** que le Core.
- Le crate Rust `DRIVER_MANAGER` définit un contrat de lancement de pilote
  isolé (`spawn_driver`, `is_task_alive`, `terminate_task`) ; aucun backend seL4
  n'y est branché.

### 1.2 Ce que veut dire "lire la majorité des matériels"

Il n'existe pas de liste d'ID à écrire une fois pour toutes. La couverture
vient de trois leviers, à combiner :

1. **Pilotes de classes standard** (un pilote = des milliers de produits) :
   AHCI, NVMe, xHCI + USB HID/stockage, HD Audio, virtio, framebuffer UEFI.
2. **Pilotes par famille** pour ce qui n'est pas standardisé : Ethernet
   (Intel e1000/igb, Realtek r8169), Wi-Fi, GPU. Chacun demande documentation,
   firmware et beaucoup de tests.
3. **Réutilisation de pilotes open source existants** via une couche de
   compatibilité (§8). C'est ce qui rend réaliste une large couverture.

Un pilote GPU Intel/AMD/NVIDIA natif complet est un chantier de longue durée.
Pour l'affichage, le framebuffer fourni par le bootloader suffit longtemps.

---

## 2. Architecture cible

```
   Énumérateurs de bus             Registre                 Attribution
 ┌─────────────────────┐    ┌───────────────────┐    ┌─────────────────────┐
 │ PCI (ports puis     │    │  BauraDevice[]    │    │  Matcher à score    │
 │ ECAM/MCFG)          │───►│  identité stable  │───►│  + liste noire      │
 │ ACPI, USB (xHCI)    │    │  ressources       │    │  + choix utilisateur│
 └─────────────────────┘    └───────────────────┘    └──────────┬──────────┘
                                                                │ candidats
                                                                ▼
 ┌────────────────────┐   ┌───────────────────┐    ┌─────────────────────┐
 │ Magasin de pilotes │──►│ Lanceur (backend  │───►│ Tâche pilote seL4   │
 │ (paquets signés)   │   │ seL4, via Rust)   │    │ avec capabilities   │
 └────────────────────┘   └───────────────────┘    └─────────────────────┘
        ▲                                                    │
        │ mise à jour (§6)                                   ▼
        └────────────── Fault/Recovery engine ◄───── santé, crash, redémarrage
```

Règles de séparation :

- Le **Core ne fait ni port I/O, ni mapping de BAR, ni attribution d'IRQ, ni
  création de capabilities seL4**. Tout cela passe par un backend de plateforme
  dans `BOOT/SEL4/`.
- Un pilote ne reçoit **que** les ressources de son appareil.
- `BauraDevice` est une structure interne, **pas** une ABI applicative.
- Le curseur de slots libres du CNode racine (`seL4_BootInfo.empty`) est
  partagé : tout code qui crée une capability prend son slot dans ce curseur
  (voir `initial_task.c`).

---

## 3. Développer un pilote BAURA

### 3.1 Deux étapes de maturité

**Étape 1 — pilote intégré (in-process).** C'est le modèle actuel
(`BauraDeviceProbe`). Rapide à écrire, aucune isolation : un bug plante le
Core. À réserver aux pilotes de démarrage (UART).

**Étape 2 — pilote isolé (tâche seL4).** Cible pour tout le reste. Le pilote est
un exécutable séparé, lancé par le gestionnaire de pilotes, qui reçoit des
capabilities explicites. Un crash n'emporte que lui, et il peut être relancé.

### 3.2 Anatomie d'un pilote isolé (proposition)

Un pilote expose un **descripteur** (ce qu'il gère, ce dont il a besoin) et un
**cycle de vie** :

```c
/* Proposition — à placer dans un en-tête partagé (SDK pilote). */
typedef struct {
    uint16_t vendor_id;      /* 0xFFFF = joker */
    uint16_t device_id;      /* 0xFFFF = joker */
    uint16_t subvendor_id;   /* 0xFFFF = joker */
    uint16_t subdevice_id;   /* 0xFFFF = joker */
    uint8_t  class_code;     /* 0xFF   = joker */
    uint8_t  subclass;       /* 0xFF   = joker */
    uint8_t  prog_if;        /* 0xFF   = joker */
    uint8_t  revision_min;
    uint8_t  revision_max;   /* 0xFF = pas de borne */
} BauraPciMatch;

typedef struct {
    uint32_t abi_version;            /* version de l'interface pilote */
    uint32_t struct_size;
    const char *name;                /* "ahci", "nvme", "xhci"… */
    uint32_t driver_version;         /* monotone, sert à la mise à jour */
    DeviceType device_type;
    const BauraPciMatch *matches;    /* table d'IDs gérés */
    uint32_t match_count;
    uint32_t required_bars;          /* masque des BAR à recevoir */
    uint32_t flags;                  /* NEEDS_DMA, NEEDS_MSI, NEEDS_FIRMWARE… */
} BauraDriverDescriptor;

typedef struct {
    BauraStatus (*probe)(const BauraDevice *dev);     /* appareil compatible ? */
    BauraStatus (*start)(BauraDriverContext *ctx);    /* initialiser, mapper */
    BauraStatus (*stop)(BauraDriverContext *ctx);     /* arrêt propre */
    void        (*on_irq)(BauraDriverContext *ctx);
} BauraDriverOps;
```

Règles :

- `probe()` **ne modifie rien** : il lit l'identité et répond "compatible /
  non". Toute initialisation va dans `start()`.
- `stop()` remet l'appareil dans un état sûr (DMA arrêté, IRQ masquées) pour
  permettre un redémarrage ou une mise à jour à chaud.
- Aucune allocation non bornée, aucun blocage dans `on_irq`.

### 3.3 Ce qu'un pilote reçoit sous seL4

Un pilote n'a **aucun accès implicite** au matériel. Le lanceur lui délègue,
selon ses besoins (noms vérifiés dans les sources de la version de seL4
épinglée par le sous-module) :

| Besoin | Mécanisme seL4 (x86_64) |
| --- | --- |
| Registres mappés en mémoire (BAR MMIO) | Untyped "device" du BootInfo (`isDevice`) → frames → mappés dans le VSpace du pilote |
| Ports d'E/S (ex. PS/2 `0x60/0x64`, UART `0x3F8`) | Capability `seL4_X86_IOPort` émise par `seL4_X86_IOPortControl_Issue` depuis `seL4_CapIOPortControl`, pour la plage exacte |
| Interruptions | Capability IRQ via `seL4_CapIRQControl` (IOAPIC ou MSI) + notification |
| Mémoire DMA | Frames dont le pilote peut obtenir l'adresse physique |
| Protection DMA | **IOMMU (VT-d)** : sans lui, un appareil peut lire/écrire toute la RAM par DMA. Le noyau seL4 épinglé ne gère que VT-d (Intel) ; aucun code IOMMU AMD n'y a été trouvé, donc **pas de confinement DMA sur AMD** (voir [`DRIVER_DOCS/05_MODELE_DE_SECURITE.md`](./DRIVER_DOCS/05_MODELE_DE_SECURITE.md)) |

Points d'attention :

- Un slot de CNode racine de l'initial task s'adresse avec la profondeur
  `seL4_WordBits` (le CNode racine a un guard), pas avec `initThreadCNodeSizeBits`.
- Le **DMA contourne les protections du processeur** : prévoir tôt l'activation
  de l'IOMMU (`-device intel-iommu` sous QEMU pour tester).
- Les BAR doivent être **mesurés** (taille, 64 bits, MMIO ou port) avant d'être
  mappés ; le registre ne conserve aujourd'hui que les dwords bruts.

### 3.4 Interface de classe (ce que voit le reste de l'OS)

Un pilote ne parle pas directement aux applications. Il s'enregistre auprès d'un
service de classe, via les interfaces déjà présentes dans le Core :

- **Stockage** : callbacks de blocs consommés par `volume_manager` (LBA
  vérifiés) puis le VFS. Le crate `USERSPACE/DRIVERS/STORAGE_DRIVERS` n'est
  **pas** un pilote : c'est une maquette. `read_block` remplit le tampon avec
  une valeur factice et affiche un message, `write_block` ne fait rien et
  retourne « succès », aucun registre n'est lu ni écrit, et le crate **ne
  compile pas** (`uintptr_t` n'existe pas en Rust ; `cargo check` donne 3
  erreurs). Son idée de points d'entrée `driver_init / driver_read /
  driver_write` peut servir de point de départ, mais il faut le réécrire pour
  le premier vrai pilote (virtio-blk, §7), en le séparant en deux moitiés (§3.6).
- **Réseau** : callback d'émission de trames consommé par `baura_network`
  (MTU validé).
- **Entrée** : événements clavier/souris vers le bus d'événements / la GUI.
- **Affichage** : framebuffer pour `GUI/DISPLAY_SERVER`.

### 3.5 Checklist d'un nouveau pilote

1. Lire la spécification (AHCI, NVMe, xHCI… sont publiques).
2. Écrire d'abord le **descripteur** (table d'IDs + besoins).
3. Écrire un **test hôte** avec un faux appareil (registres simulés), comme le
   faux hôte PCI de `BOOT/tests/pci_backend_test.c`.
4. Tester sous **QEMU** avec l'appareil émulé correspondant (§9).
5. Gérer : timeout sur chaque attente de registre, erreurs du matériel, arrêt
   propre, double `start()`.
6. Tester le crash volontaire du pilote et son redémarrage.
7. Avant tout essai sur PC réel : appliquer le protocole du §12.

### 3.6 Méthode : deux moitiés, séparées dans le code

Un pilote se découpe en deux parties qui ne se développent pas de la même façon.

| Moitié | Contenu | D'où vient le savoir | Test |
| --- | --- | --- | --- |
| **Matériel** | Registres, séquence d'initialisation, commandes, gestion des erreurs | Spécification de l'appareil, puis pilotes existants (§11) | Sur ton PC, avec un faux appareil |
| **Système** | Obtenir les registres (mapping), la mémoire DMA, les interruptions, un délai ; s'annoncer au Core | seL4 (manuel, §3.3) et ton propre Core | QEMU, puis PC réel |

La règle qui rend le tout testable : **la logique du pilote n'appelle jamais
seL4 directement.** Elle passe par une petite interface que la moitié système
implémente (proposition) :

```c
/* Proposition. La logique du pilote ne connaît que cette interface. */
typedef struct {
    uint32_t    (*read32)(void *ctx, uint32_t offset);               /* registre MMIO */
    void        (*write32)(void *ctx, uint32_t offset, uint32_t value);
    BauraStatus (*dma_alloc)(void *ctx, size_t size, BauraDmaBuffer *out); /* adresses virtuelle + physique */
    BauraStatus (*wait_irq)(void *ctx, uint32_t timeout_us);         /* attend une interruption, avec délai maximal */
    void        (*delay_us)(void *ctx, uint32_t us);
    void        *ctx;
} BauraHwAccess;
```

**Cette interface est provisoire.** Elle n'existe pas dans le Core et aucun
pilote réel ne l'a encore essayée. La version compilée et testée sur hôte est
[`DRIVER_DOCS/examples/driver_contract.h`](./DRIVER_DOCS/examples/driver_contract.h)
(elle diffère de l'extrait ci-dessus : accès aux ports `io_in/io_out`,
`map_bar`, `irq_ack`) ; en cas de divergence, c'est celle-là qui fait foi.
Attends-toi à la modifier après le premier pilote (par exemple lectures
8/16 bits, barrières mémoire, mapping de plusieurs BAR).

**Deux couches se trouvent sous le pilote, pas une :**

- **Le Core BAURA** : les contrats de classe auxquels le pilote branche ses
  callbacks (volume, réseau, entrée). Ils ont des manques, listés dans
  [`DRIVER_DOCS/04_INTERFACES_DE_CLASSE.md`](./DRIVER_DOCS/04_INTERFACES_DE_CLASSE.md).
- **seL4** : il fournit les capabilities réelles (mémoire device, IRQ, ports).
  Seule la moitié système (le remplissage de `BauraHwAccess`) en dépend ; la
  logique du pilote n'en dépend pas, donc un changement de noyau ne la touche
  pas.

C'est le schéma déjà utilisé pour le PCI (`BauraPciPortIo` dans
`BOOT/SEL4/pci_backend.h`) : la logique est dans `pci_backend.c`, l'accès seL4
dans `pci_backend_sel4.c`, et les tests simulent un hôte PCI.

Procédure pour chaque nouveau pilote :

1. Lire la spécification de l'appareil (§11) et noter les sections utiles.
2. Lire un pilote existant (BSD en premier, puis Linux) **comme référence** :
   ordre d'initialisation, pièges liés à des révisions, valeurs d'attente.
3. Écrire la logique **à partir de la spécification**, avec un commentaire qui
   cite la section suivie (par exemple « AHCI 1.3.1, §10.1.2 »). Ça rend chaque
   choix vérifiable et évite de recopier sans s'en rendre compte.
4. Écrire un faux appareil (registres simulés) et les tests hôte : succès,
   timeout, erreur du matériel, réponse incohérente.
5. Écrire la moitié système (`BauraHwAccess` pour seL4), puis tester sous QEMU.
6. Seulement ensuite, essayer sur PC réel (§12).

**S'inspirer sans copier.** Consigner dans [`DRIVER_DOCS/THIRD_PARTY_DRIVERS.md`](./DRIVER_DOCS/THIRD_PARTY_DRIVERS.md) (§8.3) chaque
pilote **consulté**, même si aucune ligne n'est reprise : origine, licence,
version. En cas de doute sur ce qui a été repris, ce registre permet de
retrouver la source. Ce document n'est pas un avis juridique.

---

## 4. Le listing des périphériques

### 4.1 Ce que collecte le registre

Pour chaque fonction PCI trouvée :

| Information | Détail |
| --- | --- |
| Emplacement | bus / slot / fonction (fonctions 1–7 explorées si l'appareil est multi-fonction) |
| Identité | vendor ID, device ID, **subsystem vendor/ID** (variantes OEM), classe, sous-classe, interface de programmation, révision |
| Ressources | BAR (dwords bruts + masque de validité), ligne d'interruption |
| Capacités | offsets des capabilities **MSI, MSI-X, PCIe** (liste parcourue de façon bornée) |
| Identifiant | `device_id` **stable** : un appareil retrouvé au même emplacement garde son identifiant et sa liaison de pilote d'un scan à l'autre |

Classification à partir de la classe PCI :

| Classe PCI | `DeviceType` |
| --- | --- |
| `0x01` (stockage de masse : SATA, NVMe…) | `DEV_TYPE_STORAGE` |
| `0x02` (réseau) | `DEV_TYPE_NETWORK` |
| `0x03` (affichage) | `DEV_TYPE_GPU` |
| `0x04`, sous-classe `0x01` ou `0x03` (audio, HD Audio) | `DEV_TYPE_AUDIO` |
| `0x06` (ponts) | `DEV_TYPE_BRIDGE` |
| `0x09` (contrôleurs d'entrée) | `DEV_TYPE_INPUT` |
| `0x0C`, sous-classe `0x03` (USB) | `DEV_TYPE_USB_HOST` |
| `0x0D` (sans fil) | `DEV_TYPE_WIRELESS` |
| autre | `DEV_TYPE_OTHER` |

Les périphériques d'entrée courants (clavier/souris USB) ne sont **pas**
visibles en PCI : ils apparaissent comme appareils USB derrière un contrôleur
xHCI, donc via un énumérateur USB (§4.3).

Capacités du registre : 256 appareils, 64 enregistrements de pilotes.

### 4.2 Règles du backend d'accès

- Le Core ne fait **aucun** accès au matériel : il reçoit un `BauraPciConfigRead`
  qui retourne non nul en cas de succès et 0 en cas d'échec.
- Le backend actuel utilise le mécanisme #1 (ports `0xCF8/0xCFC`) :
  il n'atteint que les **256 premiers octets** de configuration, et l'accès en
  deux temps (adresse, puis donnée) n'est **pas atomique** : un seul appelant à la
  fois, le backend refuse seulement un appel ré-entrant.
- Les accès invalides sont rejetés avant tout accès matériel (slot ≥ 32,
  fonction ≥ 8, offset non aligné sur 4).
- Le Core ne suppose jamais qu'une réponse est valide : une lecture qui échoue
  fait ignorer la fonction ou le champ concerné, sans bloquer le scan.

### 4.3 Prochaines étapes du listing

| Étape | Pourquoi | Jalon |
| --- | --- | --- |
| Démarrer l'initial task et vérifier la liste réelle sous QEMU | Tant que ce n'est pas fait, la détection n'est pas démontrée | M1/M2 |
| **ECAM** (table ACPI MCFG) | Accès au PCIe étendu (au-delà de 256 octets) | M3 |
| Mesure des BAR (taille, 64 bits, MMIO/port) | Nécessaire avant tout mapping ; exige une lecture/écriture de configuration et de couper le décodage pendant la mesure | M3 |
| Configuration MSI/MSI-X | Les appareils modernes l'utilisent ; les offsets sont déjà localisés | M5 |
| Événements d'ajout/retrait + rappel de retrait côté pilote | Hotplug ; aujourd'hui un appareil disparu quitte la liste sans notifier son pilote | M5 |
| Énumérateur USB (après le pilote xHCI) | Clavier, souris, stockage USB | M8 |
| ACPI (au-delà du RSDP fourni par le BootInfo) | Appareils non PCI, thermique, énergie | M8 |
| État de liaison par appareil visible (`UNSUPPORTED`, `BOUND`…) | Dire à l'utilisateur ce qui manque | M4 |

### 4.4 Vendor IDs utiles pour le catalogue

Ils servent à **cataloguer et nommer** ; ils ne disent pas qu'un pilote existe.

| Fabricant | Vendor ID PCI |
| --- | --- |
| Intel | `0x8086` |
| AMD (chipset / CPU) | `0x1022` |
| AMD / ATI (GPU) | `0x1002` |
| NVIDIA | `0x10DE` |
| Realtek | `0x10EC` |
| Broadcom | `0x14E4` |
| Qualcomm Atheros | `0x168C` |
| MediaTek | `0x14C3` |
| Samsung (NVMe) | `0x144D` |
| virtio / Red Hat | `0x1AF4` |

### 4.5 Sources de détection : d'où vient chaque information

Tous les composants d'un PC ne se trouvent pas au même endroit. Le registre PCI
n'en voit qu'une partie. Les en-têtes du BootInfo ci-dessous sont ceux de la
version de seL4 épinglée par le sous-module (`libsel4/include/sel4/bootinfo_types.h`).

| Information | Source | État dans BAURA |
| --- | --- | --- |
| Appareils PCI/PCIe (GPU, réseau, disques, USB, audio, ponts) | Configuration PCI : ports `0xCF8/0xCFC` ; ECAM via la table ACPI MCFG pour le PCIe complet | Ports : écrit, testé sur hôte, non démarré. ECAM : à faire |
| Processeur (marque, modèle, fonctionnalités) | Instruction `CPUID` ([Intel SDM](https://www.intel.com/content/www/us/en/support/articles/000006715/processors.html), [AMD APM](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24592.pdf)) | À faire |
| Cœurs et topologie | `CPUID` + table ACPI MADT ; le BootInfo donne `numNodes` | À faire |
| Mémoire RAM | BootInfo : `untypedList` (adresse, taille, indicateur `isDevice`) et en-tête étendu `SEL4_BOOTINFO_HEADER_X86_MBMMAP` | Non lu : `validate_boot_info` ne contrôle que les régions de slots |
| Écran (adresse, résolution, pas) | En-têtes étendus `SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER` et `..._X86_VBE` | Non lu |
| Fréquence du compteur de temps | En-tête `SEL4_BOOTINFO_HEADER_X86_TSC_FREQ` (en MHz) | Non lu |
| Tables ACPI | En-tête `SEL4_BOOTINFO_HEADER_X86_ACPI_RSDP` → RSDT/XSDT → MADT (CPU, IOAPIC), MCFG (ECAM), FADT, etc. | À faire |
| Batterie, ventilateurs, capteurs, boutons | Espace de noms ACPI (DSDT/SSDT : du bytecode AML). Demande un **interpréteur AML** (l'implémentation de référence s'appelle ACPICA ; ACPICA est distribué sous GPLv2, sous BSD, ou sous une licence Intel non OSI ; les paquets à double licence permettent de choisir BSD ou GPLv2 — choix à acter dans [`DRIVER_DOCS/08_DECISIONS_A_PRENDRE.md`](./DRIVER_DOCS/08_DECISIONS_A_PRENDRE.md)) | À faire, après les tables statiques |
| Périphériques I2C/SPI (pavé tactile, écran tactile, capteurs de portables) | Contrôleur (souvent visible en PCI) + description dans ACPI ; le pavé tactile est typiquement un appareil « HID over I2C » | À faire, après ACPI |
| Marque, modèle, numéro de série, barrettes de RAM | SMBIOS ([DMTF DSP0134](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.7.1.pdf)). **Pas de pointeur SMBIOS dans les en-têtes du BootInfo ci-dessus** (liste : padding, VBE, carte mémoire, RSDP, framebuffer, TSC, FDT) ; il faudrait lire la mémoire du firmware, dont la faisabilité sous seL4 est à vérifier | À faire, priorité basse |
| Périphériques USB (clavier, souris, clés, webcam, Bluetooth USB) | Pilote xHCI puis énumération USB | À faire |
| Modèle exact d'un disque | Commande `IDENTIFY` (SATA) ou `Identify` (NVMe), envoyée par le pilote de stockage | Après AHCI/NVMe |
| Codecs audio | Énumérés par le pilote HD Audio sur son propre bus | Après le pilote HD Audio |

Ordre conseillé : (1) lire le BootInfo (RAM, écran, RSDP), c'est le plus simple et
ça ne demande aucun matériel ; (2) `CPUID` ; (3) les tables ACPI statiques (MCFG,
MADT) pour passer à ECAM ; (4) USB ; (5) AML et I2C ; (6) SMBIOS.

**Définition vérifiable de « tout détecter ».** Sur la **même machine**, démarre
un Linux « live » depuis une clé séparée et relève `lspci -nn`, `lsusb`,
`lscpu` et `dmidecode`. Le critère d'acceptation de la détection est que chaque
appareil de `lspci -nn` apparaisse dans la liste de BAURA avec les mêmes
vendor/device. Sous QEMU, fais la même comparaison avec un Linux invité sur la
même machine virtuelle.

---

## 5. Attribution des bons pilotes

### 5.1 Mécanisme actuel

`attach_matching_drivers()` fait deux passes : d'abord les enregistrements par
couple exact vendor/device, ensuite ceux par type d'appareil. Dans chaque passe,
le **premier pilote dont `probe()` réussit** est retenu. Un appareil déjà lié
n'est pas re-sondé lors d'un nouveau scan.

### 5.2 Algorithme cible à score (proposition)

Pour chaque appareil, calculer un score par descripteur :

| Niveau de correspondance | Score |
| --- | --- |
| vendor + device + subvendor + subdevice (+ révision dans la plage) | 100 |
| vendor + device (+ révision) | 80 |
| vendor + classe + sous-classe + prog-if | 50 |
| classe + sous-classe + prog-if (pilote générique : AHCI, xHCI, NVMe) | 30 |
| classe + sous-classe | 10 |

Étapes :

1. Écarter les descripteurs en **liste noire** (par ID, ou par paire
   pilote/ID connu défectueux).
2. Calculer les scores ; trier par score décroissant, puis par
   `driver_version` décroissante.
3. Appliquer l'éventuel **choix explicite** (configuration ou politique).
4. Essayer les candidats **dans l'ordre** : si `probe()` ou `start()` échoue,
   passer au suivant, en journalisant la cause.
5. Si aucun ne convient : état `UNSUPPORTED`, tracé dans le journal.

### 5.3 Pilote spécifique ou générique

- Privilégier les **pilotes de classe** (AHCI, NVMe, xHCI, HD Audio), qui
  couvrent la plupart des machines.
- Ajouter des **pilotes spécifiques** seulement quand le générique est
  insuffisant ou nécessite un contournement lié à une révision.
- Appareil non reconnu : se rabattre sur le plus simple qui fonctionne
  (framebuffer pour l'écran, mode compatible pour le disque).

### 5.4 États de liaison (proposition)

```
UNBOUND ─► CANDIDATE ─► LAUNCHING ─► BOUND ─► (crash) ─► RESTARTING ─► BOUND
              │            │                                  │
              └─ UNSUPPORTED  └─ FAILED ─► candidat suivant     └─ QUARANTINED
```

`driver_attached` signifie seulement qu'un `probe()` a retourné `BAURA_OK`. Le
nouvel état doit dire si le pilote **tourne et est sain**. Après N crashs
consécutifs (valeur à fixer), le pilote est mis en quarantaine pour cet appareil ;
le moteur de récupération (`RECOVERY_ENGINE`) et le journal en gardent la trace.

### 5.5 Hotplug

1. L'énumérateur du bus émet `DEVICE_ADDED` / `DEVICE_REMOVED` via le bus
   d'événements du Core.
2. Le matcher traite l'événement pour cet appareil seul.
3. Au retrait : `stop()` du pilote, révocation de ses capabilities, libération
   de ses ressources.

---

## 6. Principe et système de mise à jour des pilotes

### 6.1 Objectifs

- Mettre à jour un pilote **sans réinstaller l'OS** et, si possible, **sans
  redémarrer** (un pilote est une tâche).
- Ne jamais pouvoir bloquer le système avec un mauvais pilote : **retour
  arrière automatique**.
- N'accepter que des pilotes **authentiques et compatibles**.

### 6.2 Le paquet de pilote (proposition)

Le dépôt possède déjà un format de paquet, **AESB**
(`PACKAGE_FORMAT/AESB_SPEC.md` : en-tête avec magic `BAURA_AESB`, version,
identifiant et hash signé ; manifeste JSON ; binaires ; ressources). Un pilote
est livré dans ce **même conteneur**, avec un manifeste de type pilote, plutôt
que dans un format distinct. La spécification AESB doit être complétée avec :

- un champ `type` (`"app"` ou `"driver"`) ;
- pour un pilote : table d'IDs gérés, ressources requises, version d'ABI
  pilote, version minimale de l'OS, firmware nécessaire, indicateur "critique au
  démarrage".

Exemple de manifeste pilote :

```json
{
  "type": "driver",
  "name": "nvme",
  "version": "1.4.2",
  "driver_abi": 1,
  "min_os": "0.3.0",
  "matches": [
    { "class": "01", "subclass": "08", "prog_if": "02" }
  ],
  "resources": { "bars": [0], "irq": "msix", "dma": true },
  "boot_critical": true
}
```

### 6.3 Le magasin de pilotes

Un dossier géré par l'OS (sur le volume système, au format de chemin VFS
`PC/<volume>:/…`) contient les paquets installés, plus un **index** `ID →
pilotes candidats` reconstruit à l'installation d'un paquet, pas à chaque
démarrage. L'index alimente le matcher (§5).

Les pilotes **critiques au démarrage** (disque système, USB clavier, écran,
UART) sont aussi présents dans l'**initrd** (`BOOT/INITRD`). Sinon, il serait
impossible de lire le magasin pour charger le pilote du disque qui le contient.

Règles de version :

- `driver_version` est **monotone** : un paquet plus ancien que celui installé
  est refusé (sauf retour arrière déclenché par le système).
- Un `driver_abi` incompatible laisse le paquet installé mais inactif.

### 6.4 Procédure de mise à jour

Le Core possède déjà un gestionnaire de mise à jour (machine à états vérifier /
appliquer / retour arrière, qui délègue aux callbacks d'un backend). On le
réutilise :

1. **Obtenir** le paquet (hors ligne ou dépôt ; le téléchargement relève du
   réseau, pas du Core).
2. **Vérifier** : signature, hash de chaque fichier, `driver_abi`, version
   supérieure, absence de la liste noire.
3. **Installer en zone temporaire** (staging) ; ne rien activer.
4. **Mettre à jour l'index** de façon atomique (écrire à côté, puis basculer).
5. **Basculer** :
   - appareil libre : `stop()` de l'ancien, lancement du nouveau ;
   - appareil en usage (disque, réseau) : plan de reprise explicite, ou
     **activation au prochain démarrage** pour les pilotes critiques.
6. **Surveiller** : chien de garde pendant une période d'essai (démarrage
   réussi, aucun crash, réponse aux requêtes de santé).
7. **Valider** la version si tout va bien ; sinon **retour arrière** vers la
   version précédente (conservée jusqu'à validation) et quarantaine de la
   nouvelle.

### 6.5 Sécurité

- **Aucun pilote non signé** par défaut. Définir la chaîne de confiance (clé
  publique du projet dans l'image, rotation de clés prévue).
- Le manifeste déclare les capabilities voulues ; le lanceur **n'en donne pas
  plus** que ce que l'appareil justifie.
- IOMMU actif pour borner le DMA (§3.3).
- Journaliser chaque installation, bascule, retour arrière et quarantaine.

### 6.6 Firmware

Le **firmware** d'un appareil (Wi-Fi, GPU, contrôleurs) est chargé par le
pilote au démarrage de l'appareil ; il voyage avec le paquet (hash propre).
Flasher le firmware de la machine (BIOS, SSD) est une opération distincte et
plus risquée, hors périmètre initial.

---

## 7. Priorités de couverture matérielle (x86_64 PC)

| Priorité | Pilote | Matériel couvert | Remarque |
| --- | --- | --- | --- |
| 1 | UART 16550 | Console de debug | **Déjà fait** (chemin legacy 32 bits) |
| 2 | Framebuffer UEFI/GRUB | Écran, tous fabricants | Transmis dans le BootInfo (en-tête `SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER`) ; suffit à la GUI |
| 3 | AHCI | Disques SATA | Pilote de classe |
| 4 | NVMe | SSD modernes | Pilote de classe |
| 5 | PS/2 clavier/souris | Portables, QEMU | `ps2_driver.cpp` existe : ports `0x60/0x64` + IRQ 1/12 |
| 6 | xHCI + USB HID + stockage | Clavier, souris, clés USB | Gros chantier, très grande couverture |
| 7 | virtio (blk, net) | Machines virtuelles | Simple, excellent pour les tests |
| 8 | Ethernet (e1000/igb, r8169) | Intel, Realtek | Les deux familles les plus courantes |
| 9 | HD Audio | Son | Pilote de classe |
| 10 | Wi-Fi, Bluetooth | Portables | Firmware, pile réseau, très long |
| 11 | GPU accéléré | Intel/AMD/NVIDIA | Très long ; Intel/AMD d'abord |

---

## 8. Sources de pilotes : open source d'abord, puis sous licence, puis propriétaire

### 8.1 Les trois phases

| Phase | Sources | Conditions | Exemples |
| --- | --- | --- | --- |
| **1 — Open source** (maintenant) | Pilotes dont le code source est publié sous licence libre | Licence compatible avec celle de BAURA (voir §8.4), traçabilité, obligations respectées | AHCI/NVMe/xHCI/e1000/HD Audio issus de projets BSD ou Linux ; modules open source NVIDIA ; piles graphiques Mesa/DRM |
| **2 — Pilotes sous licence** | Pilotes ou composants fournis dans le cadre d'**accords** (fabricant, OEM, éditeur) | Droits de redistribution **écrits**, conditions de mise à jour, support | Firmware Wi-Fi redistribuable, pilotes fournis par un partenaire matériel |
| **3 — Matériel propriétaire** | Matériel dont la pile est fermée ou la documentation non publique | Documentation ou coopération du fabricant ; chaque famille est un projet à part | GPU et chipsets dont seul le pilote fermé existe |

Chaque phase réutilise **le même mécanisme** : paquet AESB signé (§6), tâche
isolée, capabilities minimales, liste noire, retour arrière. Seule change la
**provenance** du code. Un pilote fermé reste donc dans un processus isolé et
n'a pas plus de droits qu'un pilote open source.

### 8.2 Mécanisme technique pour la phase 1

Écrire à la main tous les pilotes n'est pas réaliste. Les systèmes à
microkernel font souvent tourner du code de pilote existant en espace
utilisateur grâce à une **couche de compatibilité** :

- **Rump kernels (NetBSD)** : pilotes NetBSD exécutés en espace utilisateur.
- **Device Driver Environment (DDE)**, par exemple dans Genode : pilotes Linux
  portés avec une couche d'émulation de l'API noyau.

Le pilote réutilisé tourne comme n'importe quelle tâche pilote (§3), derrière une
interface de classe (§3.4). La couche d'adaptation est un composant à part
entière, à écrire et à maintenir.

### 8.3 Registre des composants tiers (proposition)

Aucun code tiers n'entre dans le dépôt sans une ligne dans un fichier
`THIRD_PARTY_DRIVERS.md` :

| Champ | Contenu |
| --- | --- |
| Nom, version | ex. pilote AHCI, révision précise |
| Origine | dépôt, tag ou commit |
| Licence | identifiant SPDX, **fichier par fichier** si elles diffèrent |
| Fichiers retenus | et modifications apportées |
| Obligations | notice, attribution, publication du source |
| Phase | 1, 2 ou 3 |
| État de test | matériel et émulateur sur lesquels il a été validé |

Le dépôt traite déjà ce sujet pour les icônes
(`KERNEL/L4/ROADMAP_BOOT_GUI_INTEGRATION.md` : dépôt, révision, licence, notices) ;
la même discipline s'applique aux pilotes.

### 8.4 Licences : ce qu'il faut décider avant d'importer du code

- **Le dépôt ne contient aujourd'hui aucun fichier de licence à la racine.** Il
  faut choisir la licence de BAURA **avant** d'intégrer du code tiers : c'est elle
  qui détermine ce qui est compatible.
- **Licences permissives** (MIT, BSD, ISC, Apache-2.0) : à privilégier en
  phase 1, car elles imposent peu d'obligations (notices à conserver).
- **Licences à copyleft** (GPL, LGPL) : possibles, mais leurs obligations
  (publication du source, conditions de redistribution) doivent être comprises
  avant de s'engager ; l'isolation en processus séparé aide à délimiter le
  composant concerné, sans dispenser d'une analyse.
- **Firmware** : les blobs (Wi-Fi, GPU) ont leurs propres licences,
  généralement pas libres ; ils relèvent plutôt de la phase 2.
- Ce document n'est pas un avis juridique : faire valider la politique de
  licences avant de distribuer une image contenant des composants sous
  copyleft ou sous accord commercial.

### 8.5 Où trouver du code open source par famille

| Famille | Piste (à confirmer fichier par fichier) |
| --- | --- |
| Stockage (AHCI, NVMe), USB (xHCI), Ethernet, HD Audio | Pilotes des systèmes BSD (licences BSD) en priorité ; Linux (GPLv2) quand il n'existe pas d'alternative |
| GPU Intel / AMD | Piles ouvertes Linux/Mesa : Mesa est largement sous licence MIT, mais les parties noyau mélangent des licences, à vérifier par fichier |
| GPU NVIDIA | Les modules noyau ouverts de NVIDIA sont publiés en double licence MIT/GPLv2 ([documentation NVIDIA](https://download.nvidia.com/XFree86/Linux-x86_64/595.84/README/kernel_open.html)) ; ils sont écrits pour le noyau Linux et semblent s'appuyer sur le firmware GSP (blob propriétaire), à confirmer avant de planifier un portage |
| Wi-Fi | Pilotes open source + firmware propriétaire : phase 1 pour le pilote, phase 2 pour le firmware |

---

## 9. Tests et validation

Ce guide ne peut pas établir qu'un matériel est supporté ; seuls les tests le
font. Un pilote n'est "supporté" que s'il a passé :

1. **Tests hôte** avec lecteur de configuration simulé. Existants :
   `make test-boot` (`BOOT/tests/pci_backend_test.c` : backend, adressage,
   erreurs, ré-entrance, découverte, classification, rescan, formatage) et
   `make test-core`. À ajouter : réponses malformées, BAR incohérents,
   capacité pleine.
2. **QEMU** avec appareils émulés : `-device ich9-ahci` (AHCI),
   `-device nvme,serial=…` (NVMe), `-device qemu-xhci` (USB 3), `-device e1000`,
   `-device virtio-net-pci`, `-device intel-hda` (audio), `-device intel-iommu`
   (IOMMU). `-machine q35` donne un PC PCIe moderne. `make run-sel4-x86_64`
   lance l'ISO avec `-serial stdio` : la liste affichée par l'initial task
   (`BAURA_INIT: PCI devices found: N`, puis une ligne par appareil) doit
   correspondre aux appareils ajoutés.
3. **Tests négatifs sur les capabilities** : le pilote reçoit moins que ce dont
   il a besoin → échec propre, pas de blocage.
4. **Tests de panne** : tuer le pilote, vérifier relance, quarantaine, et
   retour arrière après mise à jour défectueuse.
5. **Matériel réel**, selon le protocole du §12 : au moins un PC Intel et un AMD
   avant de déclarer le support d'une famille. Tenir une **matrice de compatibilité** (appareil,
   révision, pilote, état, date du test).

---

## 10. Feuille de route et critères d'acceptation

| Jalon | Contenu | État | Critère de réussite |
| --- | --- | --- | --- |
| M1 | Initial task seL4 qui démarre sous QEMU | À faire | Message de démarrage de l'initial task visible sur la console |
| M1b | Console visible sans port série : texte dessiné sur le framebuffer du BootInfo ; lecture du BootInfo (RAM, écran, RSDP) | À faire | Les valeurs affichées sous QEMU correspondent à la configuration de la VM (§4.5, §12.2) |
| M2 | Backend PCI seL4 + installation par l'initial task | **Code écrit et testé sur hôte** ; à vérifier au démarrage (dépend de M1) | La liste d'une machine QEMU affiche les bons vendor/device |
| M2b | Premier essai sur PC réel, **lecture seule**, selon le protocole du §12 (étages 0 à 3) | À faire | La liste PCI de BAURA couvre celle de `lspci -nn` relevée sous Linux live sur la même machine |
| M3 | Listing complet : classification, sous-IDs, capacités, identifiants stables, capacités du registre | **Fait** (testé sur hôte). Mesure des BAR écrite et testée sur hôte ([09](./DRIVER_DOCS/09_BAR_ET_MAPPING.md)) ; reste : ECAM, mapping seL4 des BAR | Tests hôte verts + comparaison avec `lspci -nn` d'une VM de référence |
| M4 | Catalogue de descripteurs + matcher à score + états de liaison (§5) | À faire | Tests : spécifique > générique, repli après échec de `probe`, liste noire |
| M5 | Backend de lancement isolé (contrat Rust → tâches seL4) + délégation de capabilities + hotplug + MSI | À faire | Un pilote trivial tourne comme tâche, crashe et redémarre sans toucher au Core |
| M6 | **Premiers pilotes open source (phase 1)** : framebuffer, AHCI ou virtio-blk, PS/2 ; registre `THIRD_PARTY_DRIVERS.md` et choix de la licence | À faire | Lecture d'un bloc de disque sous QEMU via le VFS ; clavier utilisable |
| M7 | Magasin de pilotes + paquets AESB signés + mise à jour avec retour arrière (§6) | À faire | Mise à jour réussie ; mise à jour volontairement défectueuse annulée automatiquement |
| M8 | xHCI/USB, Ethernet, audio ; IOMMU ; ACPI | À faire | Clavier USB et réseau fonctionnels sous QEMU puis sur un PC réel |
| M9 | **Pilotes sous licence (phase 2)** : accords, firmware redistribuable | À faire | Au moins un composant sous accord intégré avec ses droits de redistribution documentés |
| M10 | **Matériel propriétaire (phase 3)**, famille par famille | À faire | Une famille cible documentée, implémentée et testée sur matériel réel |

Chaque jalon met à jour `DEVICE_DRIVER_STATUS.md` : on n'y écrit "supporté" que
ce qui est **testé**.

---

## 11. Où trouver la documentation

Tous les liens ci-dessous ont été relevés par recherche au moment de la
rédaction ; vérifie leur validité avant de t'y fier, et préfère toujours la
**dernière révision** d'une spécification. Quand un lien précis n'a pas pu être
confirmé, le texte le dit.

### 11.1 Où apprendre à faire un pilote de système

| Ressource | Usage |
| --- | --- |
| [OSDev wiki — Technical Specifications](https://wiki.osdev.org/Technical_Specifications) | Index de spécifications (PCI, AHCI, xHCI, ACPI, SMBIOS, UEFI, Multiboot) avec des liens directs |
| [OSDev wiki — Common Devices](https://osdev.wiki/wiki/Category:Common_Devices) | Pages explicatives par appareil, pas à pas |
| [OSDev wiki — Accelerated Graphic Cards](https://wiki.osdev.org/GPU) | État de la documentation des GPU |
| [Linux Device Drivers, 3e éd. (LWN)](https://lwn.net/Kernel/LDD3/) | Livre en ligne sur la structure d'un pilote (écrit pour Linux : les idées valent, pas l'API) |
| [Documentation PCI du noyau Linux](https://docs.kernel.org/PCI/index.html) | Comment Linux voit le PCI/PCIe |
| [Manuel seL4](https://www.sel4.systems/Info/Docs/seL4-manual-latest.pdf) | Capabilities, IRQ, ports d'E/S, mémoire de périphérique : la moitié système |
| [Page seL4 sur IA32/x86](https://docs.sel4.systems/Hardware/IA32) | Démarrage et console sur PC réel (voir §12) |
| [seL4 Device Driver Framework (sDDF)](https://github.com/au-ts/sddf) et [sa conception](https://trustworthy.systems/projects/drivers/sddf-design.pdf) | Comment des pilotes se découpent et communiquent sous seL4 (classes réseau, bloc, série, I2C, audio). Le projet se décrit comme « largement expérimental » et s'appuie sur Microkit ; ton Core n'utilise pas Microkit, donc c'est surtout une source d'idées de conception. Sa licence n'a pas pu être confirmée ici |

### 11.2 Spécifications officielles (la source de vérité du « quoi écrire »)

| Matériel | Document |
| --- | --- |
| PC, processeur | [Intel SDM](https://www.intel.com/content/www/us/en/support/articles/000006715/processors.html) ; [AMD APM, volume 1](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24592.pdf) (les autres volumes sont sur le site d'AMD) |
| PCI, PCIe | Les spécifications officielles sont publiées par PCI-SIG (l'accès complet peut être réservé aux membres : à vérifier). Des copies de versions anciennes (PCI 2.2, PCIe 2.0) sont listées par [OSDev](https://wiki.osdev.org/Technical_Specifications) |
| Disque SATA (AHCI) | [Intel, AHCI 1.3.1](https://www.intel.com/content/www/us/en/io/serial-ata/serial-ata-ahci-spec-rev1-3-1.html) |
| SSD NVMe | [NVM Express — spécification de base](https://nvmexpress.org/specification/nvm-express-base-specification/) ; [vue d'ensemble](https://nvmexpress.org/specifications/) |
| USB 3 (xHCI) | [Intel, xHCI](https://www.intel.com/content/www/us/en/content-details/625472/extensible-host-controller-interface-for-universal-serial-bus-xhci-requirements-specification.html). Les spécifications USB et les classes (HID, stockage) sont sur [usb.org/documents](https://www.usb.org/documents?search=HID) |
| Appareils virtuels (QEMU, VM) | [virtio 1.3 (OASIS)](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html) |
| Audio | [Intel, High Definition Audio 1.0a](https://www-ssl.intel.com/content/www/us/en/standards/high-definition-audio-specification.html) |
| ACPI | [ACPI 6.6 (UEFI Forum)](https://uefi.org/specs/ACPI/6.6/) |
| UEFI (dont le framebuffer GOP) | [UEFI 2.11](https://uefi.org/specs/UEFI/2.11/) |
| SMBIOS | [DMTF DSP0134](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.7.1.pdf) |
| Démarrage Multiboot2 | [Spécification Multiboot2 (GNU)](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html) |
| PS/2 (clavier, souris) | Pas de spécification officielle récente ; les pages « Common Devices » d'OSDev sont la référence usuelle |

### 11.3 Documentation GPU : ce qui existe vraiment

| Fabricant | Ce qui est publié | Limites |
| --- | --- | --- |
| **Intel** | [Programmer's Reference Manuals](https://intel.com/content/www/us/en/docs/graphics-for-linux/developer-reference) des GPU intégrés, d'Iron Lake (2010) à Alchemist (2023), avec les chipsets associés | Les générations les plus récentes ne sont pas couvertes |
| **AMD** | [Documentation des jeux d'instructions](https://gpuopen.com/amd-isa-documentation) (RDNA, Vega…) | Ce sont surtout les instructions des shaders. À ma connaissance, pour l'initialisation matérielle (affichage, files de commandes), la source la plus complète est le code du pilote `amdgpu` de Linux : à confirmer avant de planifier |
| **NVIDIA** | [`open-gpu-doc`](https://github.com/NVIDIA/open-gpu-doc) (en-têtes de classes) et [`open-gpu-kernel-modules`](https://github.com/NVIDIA/open-gpu-kernel-modules) (pilote noyau pour Turing et suivants, double licence MIT/GPLv2 selon la [documentation NVIDIA](https://download.nvidia.com/XFree86/Linux-x86_64/595.84/README/kernel_open.html)) | La documentation officielle est partielle ; le projet [envytools](https://envytools.readthedocs.io/en/latest/hw/index.html) documente par rétro-ingénierie les générations Maxwell et antérieures. Le firmware GSP reste un blob (voir §8) |

Pour ton premier GPU, ne vise pas l'accélération : le framebuffer (§7) suffit.

### 11.4 Pilotes à lire comme référence

| Famille | Où lire | Licence (à confirmer fichier par fichier) |
| --- | --- | --- |
| AHCI | [FreeBSD `sys/dev/ahci`](https://cgit.freebsd.org/src/log/sys/dev/ahci) ; [OpenBSD `ahci_pci.c`](https://src.rcs.uwaterloo.ca/xref/openbsd-src/sys/dev/pci/ahci_pci.c) | BSD |
| NVMe | [NetBSD `nvme_pci.c`](https://nxr.netbsd.org/xref/src/sys/dev/pci/nvme_pci.c?r=1.2.2.4) (le répertoire `sys/dev/nvme` existe aussi dans FreeBSD d'après l'historique des sources ; chemin de fichier précis non vérifié) | BSD |
| xHCI (USB 3) | [NetBSD `xhci_pci.c`](https://nxr.netbsd.org/xref/src/sys/dev/pci/xhci_pci.c?r=1.26) ; [FreeBSD `xhci.c`](https://cgit.freebsd.org/src/log/sys/dev/usb/controller/xhci.c?ofs=50) | BSD |
| virtio | Le texte de la spécification virtio contient la séquence d'initialisation complète | — |
| Réseau, audio, Wi-Fi, GPU | Dossiers `sys/dev` des systèmes BSD ; le noyau Linux pour les cas absents des BSD (GPL) | Variable |

Les sites ci-dessus sont des navigateurs de code en ligne : choisis une révision
précise et note-la dans `THIRD_PARTY_DRIVERS.md`.

---

## 12. Tester sur un PC réel : protocole

Tu veux éviter toute erreur sur le PC réel. L'objectif réaliste n'est pas
« zéro erreur » (les blocages sont normaux, même pour des systèmes établis lors
d'un premier démarrage sur une nouvelle machine) mais **des erreurs sans
conséquence et faciles à diagnostiquer**. Le protocole ci-dessous vise ça.

### 12.1 Ce qui est connu sur seL4 et les PC réels

Relevé dans la [page IA32 de seL4](https://docs.sel4.systems/Hardware/IA32) et
dans une [discussion de la communauté](https://sel4.discourse.group/t/running-sel4-on-bare-metal-x86-64-pc/934) :

- Sur matériel réel, la sortie de debug passe par un **port série** : la page
  prévoit un câble série et ne mentionne pas la sortie VGA.
- La page documente des démarrages par **PXE** (réseau) ou par clé USB avec
  syslinux, en mode BIOS. Elle ne parle pas d'UEFI.
- Un utilisateur rapporte, sur une machine Xeon avec GRUB et Multiboot2 (BIOS,
  puis un essai en UEFI), un **écran noir et aucune sortie série**. La seule
  piste proposée dans le fil est le PXE ; le fil ne confirme pas de méthode
  fonctionnelle.
- Dans le tableau de plateformes de cette page, PC99 est marquée comme
  plateforme de simulation, et les lignes virtualisation et IOMMU sont vides.
  La page paraît ancienne : confirme avec la version de seL4 que tu utilises.

**Conséquence.** Le démarrage sur PC réel est le point le moins documenté de
ton plan, et il peut échouer **sans rien afficher**. Compte là-dessus plutôt que
de le découvrir sur la machine.

### 12.2 Conditions à remplir avant de brancher la clé sur un PC

1. **QEMU : la détection est correcte et stable.** L'initial task affiche la
   liste PCI attendue sur `-machine q35` **et** sur `-machine pc`, avec
   plusieurs quantités de mémoire et de cœurs (`-m`, `-smp`), sur plusieurs
   lancements de suite. `qemu-system-x86_64 -device help` liste les appareils
   émulables.
2. **Tu peux voir ce qui se passe sans port série.** La plupart des portables
   n'ont pas de port série. Le BootInfo fournit les informations du framebuffer
   (`SEL4_BOOTINFO_HEADER_X86_FRAMEBUFFER`) : fais d'abord dessiner une console
   texte sur ce framebuffer par l'initial task. Limite : elle ne montre que ce
   qui est écrit **après** le démarrage de l'initial task. Si rien ne s'affiche,
   tu ne sauras pas si le noyau seL4 ou ton initial task est en cause. Un port
   série (une machine de bureau avec un connecteur série, ou une machine avec
   console série intégrée) règle ce point ; vérifie le matériel de ta machine
   avant de compter dessus.
3. **L'image n'écrit nulle part.** Aujourd'hui, BAURA n'a **aucun pilote de
   stockage** (voir `DEVICE_DRIVER_STATUS.md`) : il ne peut donc pas écrire sur
   un disque. Garde cette propriété pour les premiers essais.

### 12.3 Règles de sécurité

1. **Machine d'essai dédiée** si tu en as une (ancien PC de bureau, idéalement
   avec le disque interne débranché). Sinon, **sauvegarde complète** de tes
   données avant.
2. **Démarre depuis une clé USB**, jamais depuis le disque interne.
3. **Aucune écriture sur un disque** tant qu'un pilote de stockage n'a pas été
   validé sous QEMU. Le premier pilote de stockage essayé sur PC réel est
   **en lecture seule** (il refuse toute commande d'écriture) et vise un disque
   de test, pas ton disque système.
4. **Ne jamais écrire** dans le flash du firmware (BIOS/UEFI), le contrôleur
   embarqué, les registres d'alimentation et thermiques, ni dans les registres
   spécifiques du processeur (MSR), sans spécification et sans test d'abord
   sous QEMU. C'est la seule catégorie d'erreur qui puisse abîmer la machine
   elle-même ; une erreur ordinaire de pilote bloque la machine ou corrompt des
   données écrites, ce qui se règle par un arrêt forcé.
5. **Écritures de configuration PCI** (registre de commande, mesure des BAR) :
   d'abord sous QEMU. Le backend actuel n'écrit sur les ports que pour
   **sélectionner** le registre lu (`0xCF8`) et ne modifie aucune configuration
   d'appareil. Sur PC réel, ajoute ces écritures dans une étape distincte, un
   appareil à la fois.
6. **Réglages du firmware** : ne change que l'ordre de démarrage. Un PC avec
   Secure Boot actif peut refuser un chargeur non signé : à vérifier sur ta
   machine, et note les réglages d'origine avant de les modifier.
7. **Sortie de secours** : arrêt forcé par le bouton, puis retrait de la clé.
   Sans écriture sur disque, la machine redémarre normalement sous son système.

### 12.4 Progression par étages

Ne passe à l'étage suivant que si le précédent a réussi sur **cette** machine.

| Étage | Contenu | Réussite |
| --- | --- | --- |
| 0 | Le noyau et l'initial task démarrent | Le message `BAURA OS Core ready` est visible (framebuffer ou série) |
| 1 | Lecture du BootInfo : RAM, framebuffer, RSDP | Les valeurs affichées correspondent à la machine |
| 2 | PCI en **lecture seule** | Chaque ligne de `lspci -nn` de la référence Linux live (§4.5) est dans la liste de BAURA |
| 3 | Tables ACPI en lecture seule | MADT et MCFG lisibles ; nombre de cœurs correct |
| 4 | Premier pilote simple (framebuffer, PS/2) | Fonctionne sans blocage |
| 5 | Stockage en lecture seule sur un disque de test | Lecture d'un secteur dont tu connais le contenu |

À chaque étape à risque, **affiche une ligne avant de l'exécuter** (par exemple
`STAGE 2: reading bus 0 slot 3`). Si la machine se bloque, la dernière ligne
affichée te dit où.

### 12.5 Ce que ce protocole ne garantit pas

Aucune méthode ne garantit l'absence d'erreur sur un matériel qu'on n'a pas
encore vu. Il garantit seulement que tu t'es donné les moyens de voir ce qui se
passe et que le pire résultat probable est un blocage corrigé par un arrêt
forcé, pas une perte de données.

---

## Annexe : hors périmètre pour l'instant

- Support ARM64/Snapdragon (chemin séparé : ACPI/device-tree + autres
  contrôleurs).
- Exposition d'une API/ABI applicative d'énumération de périphériques : à
  définir seulement une fois les contrats d'autorisation, de durée de vie, de
  hotplug et d'erreurs complets (cf. `DEVICE_DRIVER_STATUS.md`).
- Flashage de firmware machine (BIOS/SSD).
