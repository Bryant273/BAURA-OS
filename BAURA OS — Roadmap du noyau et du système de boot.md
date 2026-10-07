# BAURA OS — Roadmap du noyau et du système

> Document de référence pour les prochaines séances de développement.
> Il consigne les décisions d’architecture, l’état observé du dépôt, les
> interfaces à définir et les étapes à réaliser.
>
> **Règle :** une capacité n’est considérée comme terminée que si elle est
> intégrée au bon chemin de démarrage, validée par les contrôles prévus et
> documentée avec ses limites.

---

## 1. Vision et décisions d’architecture

BAURA OS est un système pour PC dont le noyau architectural associe **seL4** et
**BAURA Core**. Ils sont complémentaires : seL4 fournit les mécanismes de
microkernel qu’il sait déjà gérer ; BAURA Core fournit les politiques, services
et interfaces qui donnent à BAURA OS ses fonctions et son identité.

### Rôles

**seL4 fournit les mécanismes bas niveau**, selon la configuration réellement
choisie :

- tâches et mécanismes de planification ;
- espaces d’adressage et mécanismes mémoire ;
- IPC ;
- interruptions ;
- capacités et objets noyau ;
- primitives nécessaires à l’isolation.

**BAURA Core fournit les services et politiques BAURA**, au-dessus de ces
mécanismes :

- démarrage et supervision des services ;
- politique de sécurité et gestion d’identité ;
- configuration et journalisation ;
- gestion des périphériques et des pilotes ;
- stockage, volumes et VFS ;
- réseau ;
- monitoring, gestion des fautes et récupération ;
- API système BAURA.

BAURA Core ne doit pas réimplémenter en parallèle l’ordonnanceur, le VMM ou le
mécanisme de capacités de seL4. Il doit les utiliser via des interfaces
explicites et, lorsque cela convient, exécuter ses composants comme tâches ou
services isolés.

### Plateforme initiale

- Première cible : **PC x86_64**.
- Démarrage visé : firmware UEFI existant, GRUB existant, seL4 configuré pour
  la plateforme retenue, puis tâche initiale BAURA.
- UEFI et GRUB ne seront **pas réécrits**. BAURA OS doit configurer et empaqueter
  les composants existants de manière compatible.
- Le format exact de l’image seL4, les paramètres GRUB et le passage des
  informations de boot doivent être confirmés pour la configuration x86_64
  choisie. Ne pas supposer que l’image Multiboot actuelle convient.
- L’affichage d’une identité BAURA doit commencer par GRUB et la console série.
  Le rendu logiciel réutilisable de la marque et d’une splash screen est présent
  dans `GUI/UI_FRAMEWORK/baura_brand.h` et intégré aux principales surfaces GUI.
  Il n’est pas encore affiché au vrai boot : la cible intégrée doit fournir un
  framebuffer et appeler ce rendu depuis un stade de démarrage graphique.
- L’objectif initial n’est pas de prendre en charge tous les ordinateurs
  existants. Il est de construire une base reproductible, puis d’élargir la
  compatibilité sur une matrice matérielle documentée.

---

## 2. État du dépôt au moment de cet audit

Le dépôt a été publié sur GitHub dans le commit :

`84873ce10a3f79cc83a2fe8aece69b0b8b4b5e85`

Cet état sert de référence à cet audit. Vérifier le checkout et les changements
plus récents avant d’entreprendre les phases décrites ci-dessous.

### Éléments présents

- Le dépôt seL4 est référencé sous `KERNEL/L4/sel4`.
- BAURA Core comprend des modules C pour configuration, journalisation,
  événements, services, processus, ressources, sécurité, stockage, périphériques,
  monitoring, fautes/récupération, réseau, alimentation, mises à jour et
  virtualisation.
- Des implémentations Rust existent pour la sécurité et le VFS, ainsi que
  quelques éléments de gestion de drivers.
- Des fragments personnalisés existent pour l’architecture x86_64 : mémoire,
  interruptions, appels système et ordonnancement.
- L’arbre comporte des dossiers GUI, applications, installateur, shell, réseau,
  pilotes, format de paquet et compatibilité.

### Éléments qui ne sont pas encore démontrés comme fonctionnels

- Le chemin historique **x86 32 bits Multiboot** reste disponible pour les
  essais legacy. `make` construit désormais le Core x86_64, le noyau seL4 et
  l’ELF de la tâche initiale BAURA comme binaires distincts. `make
  sel4-iso-x86_64` assemble un ISO GRUB Multiboot2 qui fournit l’ELF Core
  comme module initial ; cet assemblage et son démarrage restent à vérifier
  sous Linux/QEMU.
- `BOOT/boot.asm`, `KERNEL/BAURA_CORE/kernel_main.c`, `linker.ld` et `grub.cfg`
  ne constituent pas encore le chemin de boot intégré seL4 + BAURA Core x86_64.
- `BOOT/BOOTLOADER/KERNEL_LOADER/L4_LOADER.c` contient un placeholder et ne
  démarre pas seL4.
- `BOOT/FIRMWARE/UEFI/uefi_mem.c` contient un placeholder ; il n’établit pas un
  chemin UEFI fonctionnel.
- L’adaptateur de `BOOT/SEL4` lit et valide les champs fixes de `seL4_BootInfo`
  via les headers générés de la configuration choisie. Le build et le transfert
  réel de BootInfo jusqu’à cette tâche ne sont pas encore vérifiés.
- Les fragments x86_64 de mémoire, scheduler, appels système et interruptions
  ne sont pas intégrés au build principal ; certains sont incomplets ou
  utilisent des types/fonctions non résolus dans l’arbre.
- Les modules BAURA Core sont, pour plusieurs, des registres logiciels ou des
  façades à callbacks sans backend matériel actif.
- La version Rust de la sécurité n’est pas le chemin de sécurité appelé par le
  démarrage C actuel ; son intégration au modèle de capacités seL4 reste à faire.
- Le manifeste initrd énumère des modules, mais il n’est pas démontré qu’ils
  existent tous dans une image construite.
- Le GUI, l’installateur et les applications ne sont pas intégrés au démarrage
  du noyau.
- Le catalogue annoncé de 1 000 API n’est pas encore disponible pour cet audit.
- Les commandes de build et de test présentes dans le Makefile ne constituent
  pas une validation du démarrage seL4 x86_64.

### Fichiers et dossiers de départ à garder en vue

- `Makefile`
- `build_iso.sh`
- `grub.cfg`
- `linker.ld`
- `BOOT/boot.asm`
- `BOOT/BOOTLOADER/KERNEL_HANDOFF.h`
- `BOOT/BOOTLOADER/KERNEL_LOADER/L4_LOADER.c`
- `BOOT/FIRMWARE/UEFI/uefi_mem.c`
- `BOOT/INITRD/manifest.txt`
- `KERNEL/L4/sel4/`
- `KERNEL/BAURA_CORE/README.md`
- `KERNEL/BAURA_CORE/kernel_main.c`
- `KERNEL/BAURA_CORE/SYSTEM_MANAGER/`
- `KERNEL/BAURA_CORE/SECURITY/`
- `KERNEL/BAURA_CORE/DEVICE_MANAGER/`
- `KERNEL/BAURA_CORE/DRIVER_MANAGER/`
- `KERNEL/BAURA_CORE/STORAGE/`
- `KERNEL/L4/MEMORY/`
- `KERNEL/L4/SCHEDULER/`
- `KERNEL/L4/arch/x86_64/`
- `KERNEL/L4/api/`
- `GUI/`
- `BOOT/INSTALLER/`
- `USERSPACE/DRIVERS/`

Le README de BAURA Core détaille plusieurs limites des services existants dans
`KERNEL/BAURA_CORE/README.md`.

---

## 3. Architecture visée

```text
PC et firmware UEFI existants
        |
        v
GRUB existant, démarré en mode UEFI
        |
        v
Chargement selon le protocole confirmé pour la configuration seL4 x86_64
        |
        +---- image/configuration seL4
        +---- application initiale BAURA et modules requis
        +---- informations et ressources de démarrage
        |
        v
seL4
  - mécanismes de tâches et de planification
  - mécanismes mémoire et espaces d’adressage
  - IPC
  - interruptions
  - capacités et objets noyau
        |
        v
Tâche initiale BAURA
  - valide les informations et capacités reçues
  - initialise les fonctions minimales de démarrage
  - affiche « BAURA OS » sur la console série
  - démarre les services BAURA autorisés
        |
        v
Services BAURA Core isolés et communiquant par IPC
        |
        v
Stockage, réseau, drivers, applications
        |
        v
GUI et installateur, dans des phases ultérieures
