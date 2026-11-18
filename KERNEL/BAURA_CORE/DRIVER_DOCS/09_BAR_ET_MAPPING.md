# 09 — Dimensionnement des BAR et recherche d'untyped (jalon M3, première partie)

## 1. Ce qui est écrit

| Fichier | Rôle | État |
| --- | --- | --- |
| `BOOT/SEL4/pci_backend.c` : `baura_pci_port_config_write` | Écriture d'un dword de configuration (0xCF8/0xCFC) | Testé sur hôte |
| `BOOT/SEL4/pci_bar.c` : `baura_pci_bar_probe` | Dimensionne les 6 BAR d'une fonction de type d'en-tête 0 | Testé sur hôte (faux appareil) |
| `BOOT/SEL4/pci_bar.c` : `baura_untyped_find_device` | Trouve l'untyped *device* qui contient une plage | Testé sur hôte |
| `BOOT/tests/pci_bar_test.c` | Tests ; `make test-boot` | Passe |

**Pas écrit :** le `Retype` d'untyped en frames et le `seL4_X86_Page_Map`
(décrits en `02_…` §4). Ils ne peuvent pas être testés sur hôte et rien n'a été
compilé contre les en-têtes seL4 générés ni démarré. Le code actuel ne fait que
dire **quelle plage** est utile et **quel untyped** la contient.

## 2. Procédure de dimensionnement (implémentée)

1. Lire ID, type d'en-tête, registre de commande, 6 BAR. Fonction absente
   (vendor `0xFFFF`) ou type d'en-tête ≠ 0 (ponts : types 1/2) → refus **sans
   aucune écriture**.
2. Écrire la commande avec les bits 0 (E/S) et 1 (mémoire) à 0 : l'appareil
   cesse de décoder pendant le dimensionnement.
3. Pour chaque BAR : écrire `0xFFFFFFFF`, relire, **restaurer** la valeur
   d'origine (une restauration qui échoue est retentée une fois).
4. Taille = `~masque + 1` (masque sans les bits de drapeaux : `& ~0xF` mémoire,
   `& ~0x3` E/S). BAR E/S dont les 16 bits hauts sont à 0 : taille sur 16 bits.
   BAR mémoire 64 bits : deux dwords ; si la moitié haute relit 0, la taille est
   calculée sur 32 bits. La moitié haute est rapportée `NONE`.
5. Restaurer la commande.

**Piège corrigé par un test :** le dword à l'offset 4 contient la commande
(bits 15:0) **et** le statut (bits 31:16), dont les bits sont à écrire-1-pour-
effacer. Réécrire la valeur lue effacerait des bits de statut. Le code écrit
toujours la moitié statut à 0 ; un test vérifie que le statut n'est pas modifié
(vérifié par mutation : l'ancienne version fait échouer le test).

## 3. Limites et risques — à lire avant un PC réel

- **Un appareil cesse de décoder pendant quelques accès.** Ne dimensionne pas un
  appareil qu'un autre agent utilise (firmware, SMM, contrôleur de la console
  affichée). Un framebuffer GPU qui sert à l'affichage ne doit **pas** être
  dimensionné ; le BAR du framebuffer vient du BootInfo (`02_…`).
- Les ponts et l'hôte (bus 0, 00:00.0) ne sont pas dimensionnés (type ≠ 0 ou à
  exclure explicitement) **[proposition]** : ne dimensionner que les appareils
  qu'un pilote va réellement prendre, jamais « tout le bus au démarrage ».
- Seuls les BAR sont couverts : ROM d'extension (offset 0x30), BAR des ponts,
  BAR virtuels SR-IOV, BAR MSI-X (table/PBA, dans un BAR existant) ne le sont
  pas.
- Une valeur de BAR de **0** signifie « non assigné par le firmware ». Ce code
  ne l'assigne pas ; l'attribution de plages est un chantier distinct.
- Mécanisme #1 : non atomique, 256 octets. Le backend refuse la ré-entrance mais
  ne verrouille pas (`03_…`).
- **Sécurité :** la fonction d'écriture n'est donnée qu'au code de dimensionnement
  du gestionnaire de périphériques, jamais à un pilote (`05_…` §5b).

## 4. Étapes suivantes pour mapper (non faites)

1. Pour chaque BAR utile : `baura_untyped_find_device(list, n, base, size)`.
2. Découper l'untyped si nécessaire (les objets d'un untyped sont alloués dans
   l'ordre : atteindre l'adresse voulue demande des `Retype` intermédiaires).
3. `seL4_Untyped_Retype` en `seL4_X86_4K`, puis `seL4_X86_Page_Map` avec
   `seL4_X86_CacheDisabled` (BAR de registres) ; tables de pages si nécessaire.
4. Tester sous QEMU `q35` en lisant un registre connu (ex. `device_feature` de
   la config commune virtio) et en comparant au résultat attendu.
