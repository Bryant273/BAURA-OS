# Matrice de compatibilité matérielle

Modèle à remplir **uniquement avec des résultats observés**. Une ligne = un essai
sur une machine précise (protocole : `01_DEMARRAGE_PC_REEL.md`, guide §12).

| Date | Machine (fabricant/modèle) | Firmware (BIOS/UEFI, version, Secure Boot) | Image (commit) | Démarre ? | Sortie visible (série/écran) | Liste PCI = `lspci -nn` ? | Pilotes liés | Remarques |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| _(aucun essai)_ | | | | | | | | |

Valeurs de « Démarre ? » : `oui`, `non`, `partiel` (préciser où ça s'arrête).
Ne jamais déduire une ligne d'une autre machine ou d'un émulateur : QEMU compte
comme sa propre « machine » (`QEMU q35 x.y`).
