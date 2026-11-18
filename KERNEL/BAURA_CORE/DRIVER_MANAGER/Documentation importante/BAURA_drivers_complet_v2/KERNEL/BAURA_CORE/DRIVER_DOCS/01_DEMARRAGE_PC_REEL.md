# 01 — Démarrer BAURA OS sur un PC réel

Complète le §12 de [`../DRIVER_DEVELOPMENT_GUIDE.md`](../DRIVER_DEVELOPMENT_GUIDE.md).
Légende : **[vérifié]** = lu dans une source citée ; **[proposition]** = choix de
conception ; **[à confirmer]** = non vérifié, à tester avant de s'y fier.

## 1. Ce qu'on sait, ce qu'on ne sait pas

- **[vérifié]** La documentation seL4 décrit la console d'une machine réelle
  comme un **port série**. Un rapport de la communauté signale un **écran noir
  sans aucune sortie** sur un Xeon avec GRUB Multiboot2 (BIOS et UEFI). Le PXE a
  été suggéré, sans confirmation. La page IA-32 documente PXE ou syslinux sur
  clé USB (BIOS), pas UEFI.
- **[vérifié]** `grub.cfg` du dépôt (`BOOT/SEL4/grub.cfg`) : `set timeout=0`,
  `multiboot2 /boot/kernel.elf`, `module2 /boot/baura_init.elf baura-init`.
  Avec `timeout=0` il n'y a **aucun menu** : en cas d'échec on ne voit rien.
- **[vérifié]** Le noyau seL4 n'émet le chunk framebuffer du BootInfo que si
  Multiboot2 fournit une étiquette framebuffer ; l'en-tête Multiboot2 de seL4
  (`multiboot.S`) ne contient que l'étiquette de fin, donc **aucun mode
  graphique n'est demandé**. Détail dans `02_…`.
- **[à confirmer]** Que le noyau seL4 + GRUB démarre sur *ta* machine. Rien dans
  ce dépôt n'a encore démarré, même sous QEMU.

Conséquence : **sur PC réel, la première sortie visible est le port série ou
rien.** Il faut donc un des deux : un port série réel (ou un adaptateur
USB-série branché sur un en-tête COM de la carte mère), ou une console
framebuffer (`02_…`) validée d'abord sous QEMU.

## 2. Ordre obligatoire

1. QEMU `q35` en BIOS : sortie série visible, liste PCI correcte.
2. QEMU `q35` en UEFI (OVMF) : idem, si tu vises l'UEFI.
3. Console framebuffer sous QEMU (`02_…`).
4. PC réel **sur une machine de test sans données importantes**, depuis une clé
   USB dédiée, disque interne débranché ou non monté.
5. Seulement ensuite : un second PC différent (autre fabricant de carte mère).

Ne passe pas à l'étape n+1 tant que n n'est pas reproductible deux fois de suite.

## 3. Préparer la clé USB sans risque

Le piège principal est d'écrire sur le **mauvais disque**.

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINT      # avant de brancher la clé
# brancher la clé, puis :
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINT      # la nouvelle ligne est la clé
# la clé doit avoir TRAN=usb et la taille attendue ; sinon, STOP.
sudo dd if=build/x86_64/baura_os.iso of=/dev/sdX bs=4M conv=fsync status=progress
```

Remplace `/dev/sdX` uniquement après avoir comparé les deux `lsblk`. Ne jamais
viser un disque dont `MOUNTPOINT` contient `/`, `/home` ou `/boot`.
`INSTALLATION_MANUAL.md` décrit déjà `dd` pour l'image *legacy* ; le nom du
fichier de l'image seL4 est celui produit par `make sel4-iso-x86_64` (vérifie le
nom réel dans le `Makefile`, il peut différer de `baura_os.iso`).

Vérifier que l'ISO est amorçable en BIOS et/ou EFI **avant** de l'écrire :

```sh
xorriso -indev build/x86_64/<image>.iso -report_el_torito as_mkisofs
```

La sortie liste une entrée BIOS (`-b …`) et/ou une entrée EFI (`-e …` /
`-eltorito-alt-boot`). Une image `grub-mkrescue` standard contient en général les
deux si les paquets GRUB EFI sont installés **[à confirmer sur ton environnement
de build]** ; sans entrée EFI, la clé ne démarrera pas en mode UEFI pur.

## 4. BIOS (CSM/Legacy) ou UEFI ?

| Mode | État | Remarque |
| --- | --- | --- |
| BIOS / CSM | Le plus proche de ce que la doc seL4 décrit (PXE/syslinux) | Beaucoup de PC récents n'ont plus de CSM |
| UEFI | Non documenté par seL4 pour IA-32 ; le rapport communautaire d'écran noir couvre aussi l'UEFI | À tester d'abord sous OVMF |

**[proposition]** Commence par une machine avec CSM activable. Note dans
`COMPATIBILITY_MATRIX.md` le mode utilisé à chaque essai.

## 5. Secure Boot

**[vérifié]** Avec Secure Boot actif, seuls des logiciels signés s'exécutent
(base de connaissances Framework). GRUB et le noyau seL4 de ce dépôt ne sont pas
signés par une clé de confiance. **[proposition]** Désactive Secure Boot pour les
essais ; ne l'enregistre pas comme « solution » : c'est un choix de sécurité à
documenter dans `08_DECISIONS_A_PRENDRE.md`. Pense à noter si BitLocker /
chiffrement du disque existant demande une clé après changement de réglage
firmware, **avant** de modifier quoi que ce soit.

## 6. Rendre l'échec visible

**[proposition]** pour l'image de test PC réel uniquement :

```
set timeout=10
set default=0
menuentry "BAURA OS (seL4)" { multiboot2 /boot/kernel.elf
                              module2 /boot/baura_init.elf baura-init
                              boot }
```

(adapter à la syntaxe exacte de `grub.cfg` du dépôt). Le menu donne aussi accès
à la ligne de commande GRUB (`c`) pour inspecter `ls`, `lsmmap`, `videoinfo`
**[à confirmer : disponibilité des commandes selon le build de GRUB]**.

Pour obtenir un framebuffer, GRUB a une variable `gfxpayload` (`set
gfxpayload=keep` ou une résolution) qui contrôle le mode transmis à l'OS
**[à confirmer : comportement exact avec `multiboot2` et seL4 ; à tester sous
QEMU en lisant si le noyau affiche « Got framebuffer info in multiboot2 »]**.

## 7. Arbre de décision « écran noir »

1. Le menu GRUB s'affiche-t-il ? Non → problème de clé/firmware (§3, §4, §5).
2. Oui, mais rien après la sélection → le noyau n'a probablement pas de sortie
   visible (attendu, §1). Brancher le série ; sinon passer à la console
   framebuffer.
3. Série branché, rien → vérifier le câble croisé (null-modem), 115200 8N1, le
   bon COM (0x3F8 est COM1), et que le firmware n'a pas désactivé le port.
4. Le noyau imprime, l'initial task non → regarder la dernière ligne imprimée ;
   comparer avec le journal QEMU ; ne pas conclure depuis un seul essai.
5. Le noyau et l'initial task impriment, liste PCI incomplète → comparer à
   `lspci -nn` d'un Linux live **sur la même machine** (critère d'acceptation du
   guide §4.5).

## 8. Ce qu'on ne fait pas sur un PC réel au début

Aucun test d'écriture sur un disque ; aucune écriture de configuration PCI
(le backend ne fait que lire) ; pas de mise à jour du firmware ; pas de test
sur la machine de travail principale.

## 9. Journal d'essai (à copier pour chaque essai)

```
Date / machine (fabricant, modèle, BIOS version) :
Mode firmware (BIOS/UEFI), Secure Boot (on/off) :
Image (commit, hash sha256) :
Clé USB (modèle) :
Sortie observée (photo / capture série) :
Résultat attendu / obtenu :
Comparaison avec lspci -nn :
```
