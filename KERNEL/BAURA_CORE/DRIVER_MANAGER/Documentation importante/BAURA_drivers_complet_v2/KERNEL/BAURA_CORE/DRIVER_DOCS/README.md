# Documents complémentaires du guide des pilotes

Point d'entrée : [`../DRIVER_DEVELOPMENT_GUIDE.md`](../DRIVER_DEVELOPMENT_GUIDE.md).
Statut de ce qui existe : [`../DEVICE_DRIVER_STATUS.md`](../DEVICE_DRIVER_STATUS.md).

| Doc | Sujet |
| --- | --- |
| [01](./01_DEMARRAGE_PC_REEL.md) | Procédure de démarrage sur PC réel |
| [02](./02_BOOTINFO_ET_CONSOLE_FRAMEBUFFER.md) | BootInfo et console framebuffer (M1b) |
| [03](./03_MODELE_DE_CONCURRENCE.md) | Modèle de concurrence |
| [04](./04_INTERFACES_DE_CLASSE.md) | Contrats d'interface par classe |
| [05](./05_MODELE_DE_SECURITE.md) | Capabilities, DMA, niveaux de confiance |
| [06](./06_PLAN_VIRTIO_BLK.md) | Premier pilote : virtio-blk |
| [07](./07_PAQUETS_ET_MISES_A_JOUR.md) | Paquets, signature, retour arrière, pannes |
| [08](./08_DECISIONS_A_PRENDRE.md) | Décisions ouvertes |
| [09](./09_BAR_ET_MAPPING.md) | Dimensionnement des BAR, recherche d'untyped (M3, partie 1) |
| [THIRD_PARTY_DRIVERS](./THIRD_PARTY_DRIVERS.md) | Registre de provenance (modèle) |
| [COMPATIBILITY_MATRIX](./COMPATIBILITY_MATRIX.md) | Matrice d'essais (modèle) |

Convention : **[vérifié]** = lu dans une source citée ; **[proposition]** =
choix de conception ; **[à confirmer]** = à tester ou relire avant usage.
`examples/` contient du code C **compilé et testé sur hôte** (`make
test-driver-docs`) ; ce ne sont pas des modules du Core.
