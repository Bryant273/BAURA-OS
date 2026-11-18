# 04 — Contrats d'interface par classe de périphérique

Un pilote n'est utile que s'il expose au Core une interface précise. Ce document
dit **ce qui existe** (lu dans le code) et **ce qui manque**. Les structures de
[`examples/driver_contract.h`](./examples/driver_contract.h) sont des
propositions compilées sur hôte, pas du code du Core.

## 1. Principe

Étape 1 : callbacks C dans le même espace d'adressage (c'est ce que le Core
fait aujourd'hui). Étape 2 (pilotes isolés) : les mêmes opérations transportées
par IPC et par des anneaux en mémoire partagée (`03_…`). Les contrats doivent
donc être exprimables sans pointeurs partagés entre espaces d'adressage.

## 2. Stockage bloc

**Existe :** `BauraBlockDevice {context, block_count, block_size, read, write}`,
`BauraBlockRead/Write(context, lba, block_count, buffer)`.
**Manque :**
- indicateur **lecture seule** (nécessaire pour le premier test virtio-blk et
  pour tout support protégé) ;
- opération **flush** ;
- **désenregistrement** : le gestionnaire de volumes n'a que
  register/get_id/read/write/count, donc un disque retiré ou un pilote planté
  laisse un volume fantôme ;
- erreurs distinctes (média absent, I/O, hors limites) ; taille de bloc ≠ 512.
**Proposition :** ajouter `flags` (RO), `flush`, et `volume_unregister(id)`
avant le premier pilote réel.

## 3. Réseau

**Existe :** uniquement l'émission : `BauraNetworkTransmit(context, frame,
length)`, `BAURA_NETWORK_FRAME_MAX` = 1518.
**Manque :** **chemin de réception**, adresse MAC, état du lien, statistiques,
trames > 1518 (VLAN, jumbo), enregistrement/retrait d'une interface. Sans RX,
aucun pilote réseau n'est testable de bout en bout.

## 4. Entrée (clavier/souris)

**Existe :** `GUI/INPUT/ps2_driver.h` : `BauraPs2Io {read8, write8, context}`,
ports 0x60/0x64, événements KEY/MOUSE. **Proposition :** publier les
événements sur le bus d'événements du Core (synchrone, charge de 4 mots) —
acceptable pour des événements d'entrée peu volumineux ; le délai de traitement
des abonnés doit rester borné. USB HID suivra le même format d'événement.

## 5. Affichage

**Existe :** rien côté pilote ; le rendu logiciel est dans `GUI/`. **Contrat
minimal (proposition) :** `{base, width, height, pitch, bpp, format}` fourni par
le BootInfo (`02_…`), plus `flush(rect)` facultatif. Un pilote GPU natif n'est
pas nécessaire avant très longtemps (guide §1.2).

## 6. Cycle de vie commun

`BauraDriverOps {probe, start, stop}` + états de liaison `BauraBindState` (NONE,
CANDIDATE, PROBING, BOUND, FAILED, QUARANTINED). Règles : `probe` ne modifie
rien d'irréversible ; `stop` libère toute ressource et est idempotent ; le
retrait d'un appareil appelle `stop` (le registre n'a **pas encore** de
callback de retrait : voir `DEVICE_DRIVER_STATUS.md`).

## 7. Table de correspondance

`BauraMatchEntry` + `baura_match_score` : plus la règle est précise, plus le
score est haut (vendor/device 8+16, sous-système 4+4, classe/sous-classe/prog-if
1 chacun) ; une règle qui contredit l'appareil renvoie -1. Testé sur hôte :
une règle exacte bat une règle de classe. Ces poids sont un **choix à valider**,
pas une norme.
