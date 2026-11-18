# 03 — Modèle de concurrence

## 1. Constats — [vérifié dans le dépôt/sources]

- Le Core est **mono-cœur et non synchronisé** (README). Le bus d'événements
  appelle les abonnés **synchroniquement** (32 max, charge utile de 4 mots) et
  n'est pas sûr pour des appelants concurrents.
- seL4 `KernelMaxNumNodes` vaut **1** par défaut : une seule CPU est utilisée.
- Une IRQ seL4 est livrée comme **signal sur une notification** ; le pilote la
  reçoit avec `seL4_Wait`/`seL4_NBWait` puis acquitte avec
  `seL4_IRQHandler_Ack`.

## 2. Règles — [proposition]

1. **Le Core reste un seul fil d'exécution** : une boucle d'événements dans
   l'initial task. Aucun appel du Core depuis une autre tâche.
2. **Un pilote isolé = une tâche seL4** (plus tard). Il parle au Core par IPC
   ou par un anneau en mémoire partagée, jamais en appelant le Core.
3. **Interruption** : le pilote attend sur sa notification ; à réception il
   traite l'événement, puis `Ack`. Jamais de travail long avant l'`Ack` d'un
   IRQ partagé/niveau, sinon tempête d'interruptions.
4. **Pas de blocage dans le Core** : toute attente matérielle est bornée par un
   délai ; échec → `BAURA_ERR_*`, pas de boucle infinie.
5. **Anneaux SPSC** (un producteur, un consommateur) pour les données entre le
   Core et un pilote : [`examples/spsc_ring.h`](./examples/spsc_ring.h),
   capacité puissance de 2, `__atomic` acquire/release, testé sur hôte (plein,
   vide, ordre). Ce n'est correct que pour **un seul** producteur et **un seul**
   consommateur.
6. **Temps** : `delay_us` s'appuie sur le TSC. La fréquence est fournie par le
   chunk BootInfo n° 5 (MHz). Si absent, utiliser une boucle bornée prudente et
   le signaler ; ne pas supposer un TSC invariant sans le vérifier
   (CPUID) **[à confirmer]**.
7. **Concurrence PCI** : le mécanisme #1 (0xCF8/0xCFC) n'est **pas atomique**.
   Seul le gestionnaire de périphériques y accède (le backend refuse déjà la
   ré-entrance). Un pilote ne reçoit jamais le port 0xCF8.

## 3. Quand reconsidérer

Passage multi-cœur (`KernelMaxNumNodes` > 1) : il faudra alors verrouiller le
Core ou le réserver à un cœur, revoir les anneaux (un anneau par paire de
cœurs) et l'affinité des IRQ. Hors périmètre tant que le boot mono-cœur n'est
pas validé.
