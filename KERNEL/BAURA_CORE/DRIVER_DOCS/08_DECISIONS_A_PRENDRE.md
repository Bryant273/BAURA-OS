# 08 — Décisions à prendre (et conséquences)

Aucune de ces décisions n'est prise dans le dépôt. Chacune bloque ou oriente
des pilotes. Je donne des faits, pas un avis juridique.

| # | Décision | Faits utiles | Bloque |
| --- | --- | --- | --- |
| 1 | **Licence du projet** | Le noyau seL4 est sous **GPLv2** ; le code utilisateur de seL4 est **BSD-2-Clause** ; le code BAURA en espace utilisateur n'est pas couvert par la GPL du noyau. Le choix de la licence BAURA détermine quel code externe on peut copier | Réutilisation de code de pilotes |
| 2 | Couche de compatibilité vs écriture de zéro | Compat (type Linux/BSD) : large couverture, grosse surface à maintenir, licence GPL à traiter. De zéro : propre, lent | Couverture Ethernet/Wi-Fi |
| 3 | Algorithme de signature | Proposition Ed25519 + SHA-256 (`07_…`) | Mises à jour |
| 4 | Méthode d'amorçage cible | BIOS+GRUB, UEFI ; Secure Boot (`01_…`) | PC réel |
| 5 | Politique DMA sans IOMMU (AMD) | Aucun IOMMU AMD trouvé dans les sources seL4 épinglées (`05_…`) | Pilotes tiers |
| 6 | Seuils de panne | 3/60 s proposés sans mesure (`07_…`) | Reprise |
| 7 | Adopter ACPICA ? | ACPICA : GPLv2, BSD, ou licence Intel non OSI ; les paquets à double licence permettent de choisir BSD ou GPLv2 | ACPI/AML (batterie, thermique) |
| 8 | Langue/convention de docs | Docs du dépôt en langues mixtes ; ce guide est en français, `DEVICE_DRIVER_STATUS.md` en anglais | Cohérence |
| 9 | Politique d'inspiration vs copie | S'inspirer = comprendre le matériel puis écrire ; copier du code GPL le rend dérivé | Licence |

## Règle de provenance — [proposition]

Chaque pilote porte en en-tête : source de la documentation (URL/version),
licence, et « écrit sans copier le code de X » ou « dérivé de X (licence) ».
`THIRD_PARTY_DRIVERS.md` le centralise.
