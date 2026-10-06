# ABI système BAURA

La spécification de départ est [API_ABI_v2.json](../SYSTEM_API/API_ABI_v2.json)
(version sémantique 2.0). Les types C, statuts, erreurs, capacités et
identifiants des cinq opérations de référence sont exposés par
[baura_abi_v2.h](../SYSTEM_API/baura_abi_v2.h).

Le protocole IPC v2.0 définit les trames, opcodes, statuts/erreurs numériques,
pièces jointes mémoire et charges utiles des cinq opérations de référence.
Le codec C byte-à-byte est dans `SYSTEM_API/baura_ipc_v2.c` et ses tests hôtes
s'exécutent avec `make test-abi`. La cible native est PC x86_64; le codec du
format wire reste indépendant de la largeur des pointeurs hôtes.

Cela fournit un contrat et un codec testable, pas encore des implémentations
appelables ni un transport seL4 intégré. La convention d'appel
`BAURA_ABI_DEFAULT` reste à lier à une toolchain ABI x86_64 concrète. Les
limites et les étapes d'intégration sont détaillées dans
[API_ABI_v2_STATUS.md](../SYSTEM_API/API_ABI_v2_STATUS.md).