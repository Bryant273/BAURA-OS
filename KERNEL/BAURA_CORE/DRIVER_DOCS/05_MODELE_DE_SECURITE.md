# 05 — Modèle de sécurité des pilotes

Un pilote est la partie la plus dangereuse du système : il parle à du matériel
qui peut lire/écrire la mémoire. Ce document sépare **faits vérifiés**,
**limites** et **propositions**.

## 1. Faits — [vérifié dans les sources seL4 épinglées]

- Les pilotes sont des tâches en espace utilisateur ; l'accès au matériel passe
  par des **capabilities** issues du BootInfo : `seL4_CapIOPortControl` (7),
  `seL4_CapIRQControl` (4), `seL4_CapIOSpace` (8) et les untypeds *device*.
- Ports : `seL4_X86_IOPortControl_Issue(…, first, last, …)` crée une capability
  limitée à une plage ; IRQ : `seL4_IRQControl_GetIOAPIC` / `GetMSI` ;
  `seL4_IRQHandler_SetNotification`/`Ack`.
- **IOMMU : `KernelIOMMU` = « support IOMMU pour chipsets VT-d »**, activé par
  défaut sur pc99 hors build de vérification. **Aucun code IOMMU AMD n'a été
  trouvé** dans les sources épinglées → sur une machine AMD, **rien ne
  confine le DMA d'un périphérique**.

## 2. Conséquences

1. Un pilote ne peut accéder qu'aux ports/IRQ/frames **dont il a la capability**.
2. Un périphérique capable de DMA peut lire/écrire **n'importe quelle mémoire
   physique** tant qu'aucun IOMMU n'est configuré. Sur Intel avec VT-d, on peut
   le restreindre via `seL4_CapIOSpace` ; sur AMD (ou VT-d non configuré) un
   pilote malveillant/bogué *équivaut à du code noyau*.
3. Aujourd'hui, **`process backend start(context, name, image_path, arguments,
   parent_task_id, &task_id)` n'a aucun paramètre de délégation de ressources**
   : il ne peut pas encore donner à une tâche pilote ses capabilities.

## 3. Matrice des capabilities par classe — [proposition]

| Classe | Ports | IRQ | Mémoire device | DMA | Config PCI |
| --- | --- | --- | --- | --- | --- |
| virtio-blk | non | 1 (MSI-X/ligne) | BAR 4 (modern) | oui | lecture seule via gestionnaire |
| AHCI/NVMe | non | 1+ | BAR | oui | lecture seule |
| NIC | éventuellement (anciens) | 1+ | BAR | oui | lecture seule |
| PS/2 | 0x60, 0x64 | 1, 12 | non | non | n/a |
| Framebuffer | non | non | plage fb | non | non |
| Série (debug) | plage COMx | optionnelle | non | non | n/a |

Principe : **moindre privilège**, une capability par ressource, jamais
`IOPortControl` entier au pilote.

## 4. Niveaux de confiance — [proposition]

- **Niveau 0** : code du Core + initial task (confiance totale, petit).
- **Niveau 1** : pilotes signés BAURA, isolés (tâche + capabilities limitées).
- **Niveau 2** : pilotes tiers du registre (`THIRD_PARTY_DRIVERS.md`) : mêmes
  contraintes, installation explicite par l'utilisateur, **jamais** si le DMA
  n'est pas confiné, sauf décision écrite (`08_…`).

## 5. Politique DMA — [proposition à décider]

- Intel avec VT-d actif : DMA confiné par le noyau ; pilotes de niveau 1/2
  autorisés.
- AMD / sans IOMMU : afficher un avertissement au démarrage ; n'autoriser que
  les pilotes de niveau 0/1 audités ; refuser le niveau 2. Détecter la présence
  de l'IOMMU par ACPI (tables DMAR pour Intel) **[à confirmer]**.

## 5b. Écritures de configuration PCI

Le backend actuel ne fait que **lire**. Toute écriture (commande bus-master,
BAR, MSI) doit passer par le gestionnaire de périphériques, qui valide qu'elle
concerne l'appareil du pilote et filtre les registres dangereux (BAR d'un autre
appareil, bridges).

## 6. Journal et pannes

Chaque refus de capability, plantage et quarantaine est un enregistrement du
moteur de fautes (`BauraFaultRecord`, capacité 32 : à agrandir si on veut un
historique utile). Politique de redémarrage : `07_…` §6.
