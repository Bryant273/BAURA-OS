# Roadmap — sécurité, mémoire, réseau et alignement API/ABI

## But et limites

Cette feuille de route définit le développement des services sécurité, mémoire
et réseau de BAURA Core, et leur alignement avec l’API/ABI BAURA et l’interface
de plateforme seL4. Elle complète les roadmaps du démarrage et du microkernel ;
elle ne remplace ni ne modifie la roadmap historique du système.

Le résultat visé est une architecture où les applications et services BAURA
utilisent des contrats stables, le Core applique les politiques de l’OS et
seL4 applique les mécanismes d’isolation et d’autorité. Les services Rust ou
Go ne sont pas des raccourcis vers les objets noyau : tout droit réel doit
provenir d’un backend de plateforme lié à la configuration effectivement
démarrée.

## Invariants d’architecture

1. **seL4 — mécanismes.** Le noyau gère les espaces d’adressage, frames,
   threads, IPC, capacités, notifications et IRQ. Les bibliothèques BAURA ne
   simulent pas ces objets en leur donnant des identifiants entiers.
2. **BAURA Core — politique et supervision.** Le Core décide si une opération
   BAURA est permise, applique quotas/règles, supervise les services et expose
   une API stable. Il appelle un backend typé pour les effets qui exigent le
   noyau ou un périphérique.
3. **Rust — composants de politique/services sûrs.** Rust peut héberger les
   services de sécurité BAURA, les validateurs et éventuellement des services
   mémoire. Il n’est une couche de protection supplémentaire que si son
   processus est isolé par le noyau et ses ressources lui sont transférées
   avec les droits appropriés. Une table en mémoire du processus ne constitue
   pas une CSpace seL4.
4. **Go — services de protocole de haut niveau.** Go peut convenir à des
   services réseau utilisateur (protocoles, gestion de connexions, politique
   applicative), à condition de disposer d’un runtime compilable pour le
   modèle de tâche retenu, d’une isolation seL4 et d’un transport ABI. Il ne
   remplace pas les pilotes Wi-Fi/Bluetooth, les mécanismes radio, ni la
   configuration des interruptions/DMA.
5. **API/ABI BAURA — contrat d’application.** Les consommateurs appellent les
   opérations BAURA, pas les syscalls seL4. Le binding seL4 reste confiné à
   l’adaptateur de plateforme. Les capacités passent comme objets transférés
   par le noyau, jamais sous forme de pointeurs ou de nombres sérialisés qui
   accorderaient une autorité.
6. **Séparation des statuts.** Une opération spécifiée, un codec testé, un
   mock, un callback injecté, un backend seL4 et un service démarré sont des
   états différents. La documentation et les erreurs doivent les distinguer.
7. **Pas de promesse de sécurité non démontrée.** « Firewall », « Defender »,
   « mémoire sécurisée » ou « sandbox » ne sont pas déclarés opérationnels
   avant d’identifier les données observées, les droits, les backends et les
   essais négatifs correspondants.

## Inventaire initial

État observé dans le dépôt avant les travaux de cette roadmap :

| Domaine | Contrat ou code présent | Écart à combler |
| --- | --- | --- |
| ABI générale | `SYSTEM_API/API_ABI_v2.json`, header C et codec C pour les cinq opérations de référence | Aucun transport seL4, dispatcher ou stub callable ; seules cinq opérations sont définies |
| Processus | Le Core a une interface injectable de cycle de vie | Pas de backend seL4 ni de création intégrée de service |
| Sécurité C | Politique logicielle de droits et callback d’autorité du Core | Tables C distinctes des capacités seL4 ; source d’autorité réelle non liée |
| Sécurité Rust | Crate Rust à CSpaces logiciels en mémoire | Duplication du modèle C, `Mutex` avec `unwrap`, pas de liaison seL4/ABI |
| Drivers Rust | Contrat de backend plateforme versionné pour spawn, liveness et terminaison ; erreur si absent | Aucun backend seL4 n’est lié ; tests hôtes seuls ne prouvent pas l’isolation |
| Mémoire | Opération ABI `MEM-0001` et quotas de ressources comptables | Pas d’allocateur/mapping seL4 ou de service mémoire exposé par IPC |
| Réseau C | Façade d’envoi de trames via callback | Pas de backend NIC/stack complet ou d’opérations ABI de gestion réseau |
| Bus Go | Bus de callbacks Go utilisant cgo | Pas de transport IPC BAURA, processus isolé ou pilote ; ce n’est pas un firewall |
| Démarrage | Archive Core x86_64 et cible de build noyau seL4 distinctes | Pas de tâche initiale BAURA intégrée ni de binding configuré à l’exécution |

Cette table décrit les fichiers, non une validation de build ou de runtime.

### Opérations ABI v2 déjà définies

| ID / opcode | Codec et forme définie | Service/backend réel dans le dépôt |
| --- | --- | --- |
| FS-0001 / `0x00010001` | Payload create validé par le codec C | Pas de filesystem monté ni dispatcher IPC |
| FS-0002 / `0x00010002` | Payload read et attachement décrit/validé par le codec C | Pas de filesystem monté, transport de frames ni dispatcher |
| PR-0001 / `0x00020001` | Payload path/arguments/options validé par le codec C | Pas de création de tâche seL4 intégrée |
| MEM-0001 / `0x00030001` | Forme requête/réponse et attachement décrits par ABI/codec | Pas d’allocateur de frames ou capability mémoire de réponse |
| NET-0001 / `0x00040001` | Payload endpoint/options validé par le codec C | Pas de service de connexion ou pile réseau raccordée |

Les statuts « codec » font référence aux fonctions et tests hôtes prévus par
`baura_ipc_v2.c` et `make test-abi` ; la réussite des tests doit être constatée
dans l’environnement de build avant de la déclarer. Le backend reste absent
même si une frame est correctement encodée.

## Étapes de réalisation

### Étape 0 — établir une base mesurable

1. Exécuter les suites déjà documentées : `make test-core`,
   `make test-core-x86_64`, `make test-rust`, `make test-communication` et
   `make test-boot`. Garder les échecs préexistants séparés des nouveaux.
2. Inclure tous les crates Rust de Core dans leur cible de test, notamment le
   gestionnaire de drivers.
3. Ajouter un état de statut par opération ABI : `specified`, `codec`,
   `host-backend`, `platform-backend`, `integrated`, `tested-on-target`.

**Sortie :** inventaire reproductible sans confondre test hôte avec exécution
seL4.

### Étape 1 — aligner les sources normatives et les bindings

1. Garder `API_ABI_v2.json` comme contrat normatif sémantique et de fil.
   Vérifier sa validité JSON et auditer chaque opération : domaine, opcode,
   payload, attaches, capability requise, erreurs, blocking et mapping système.
2. Comparer systématiquement le JSON, `baura_abi_v2.h`, l’encodeur/décodeur C,
   le dispatcher futur, le header FFI Rust et les clients Go.
3. Définir pour chaque différence si elle est une conversion de langage valide
   ou une incompatibilité. L’ABI native de fonction ne doit pas être confondue
   avec le wire ABI.
4. Générer les constantes/bindings répétitifs à partir de la source normative,
   ou ajouter un test de conformité qui détecte toute divergence. Ne jamais
   générer une opération dont le contrat n’est pas complet.
5. Versionner les nouvelles opérations (sécurité, mémoire, événements et
   réseau) avec des payloads, règles d’attachement, erreurs et effets
   explicitement définis avant de coder leurs stubs.

**Sortie :** une seule définition sémantique, bindings vérifiables et aucune
interface Go/Rust qui utilise des conventions privées incompatibles.

Un garde-fou est ajouté : `SYSTEM_API/check_abi_v2.py` est appelé par
`make test-abi` et compare le JSON normatif aux constantes publiques C
(identifiants, opcodes, offsets, magic, version, limites, flags/réservés,
types/statuts de message et erreurs). Il vérifie également les tables JSON
d'opcodes/statuts dupliquées, les symboles ABI, le nombre d'attachements et le
mode bloquant des opérations de référence par rapport au codec. Ce contrôle
n’est pas un générateur de bindings et doit encore être exécuté dans
l’environnement de build.

### Étape 2 — transport et premier chemin ABI réel

1. Confirmer le contrat réel de tâche/BootInfo/capacités du démarrage seL4
   retenu, conformément à
   [`ROADMAP_BOOT_GUI_INTEGRATION.md`](../L4/ROADMAP_BOOT_GUI_INTEGRATION.md).
2. Implémenter l’adaptateur de transport IPC seL4 hors du Core portable,
   avec des erreurs explicites et une sémantique attach/détach vérifiée.
3. Créer un dispatcher qui valide toute la frame et les capacités transférées
   avant d’appeler un backend ou de provoquer un effet.
4. Connecter un seul appel de bout en bout, d’abord choisi parmi les services
   dont le backend existe. Ajouter les cas malformed frame, droits insuffisants,
   timeout et backend absent.

**Sortie :** un client utilisateur effectue une requête ABI sur l’IPC réel à un
service BAURA isolé ; les tests codec seuls ne suffisent pas.

### Étape 3 — définir l’autorité et consolider la sécurité Rust

1. Définir la frontière entre politique BAURA (identités, rôles, règles et
   décision) et autorité de noyau (capabilities/CSpaces seL4).
2. Choisir un seul propriétaire de la politique : éviter deux bases de
   capabilities mutables divergentes en C et Rust. Documenter la migration,
   l’API interne et le format des erreurs.
3. Convertir le crate Rust en bibliothèque de politique/service compatible
   avec le runtime réellement choisi. Ses APIs ne mintent ni ne délèguent une
   capability noyau sans appels de backend autorisés.
4. Faire échouer explicitement les opérations exigeant un backend absent.
   Remplacer les panics FFI/locks non gérés, ajouter limites de capacité et
   tests de concurrence/fuzzing sur les entrées non fiables.
5. Définir le modèle du Defender : sources observables, provenance des règles,
   mises à jour vérifiables, décisions, journalisation et limites. Ce service
   ne reçoit que les événements/données délégués.

**Sortie :** un service de politique qui traite des demandes d’autorisation
réelles via ABI et dont l’autorité effective est contrôlée par seL4.

### Étape 4 — allocation et mapping mémoire

1. Séparer comptabilité/quota Core, politique du service mémoire et primitives
   d’allocation/retype/map fournies par le noyau.
2. Définir les contrats de frames, alignement, tailles, droits, durée de vie,
   restitution et transfert IPC sans exposer d’adresse physique ou de pointeur
   utilisateur comme autorité.
3. Implémenter un backend x86_64 selon le BootInfo et les capacités réellement
   reçues ; traiter fragmentation, manque de mémoire et nettoyage partiel.
4. Implémenter `MEM-0001` suivant ses attaches ABI, puis définir de nouvelles
   opérations seulement si un besoin distinct est établi.
5. Tester overflow, alignement, accès interdit, épuisement des frames,
   révocation et cas d’échec après allocation partielle.

**Sortie :** requête ABI allouant/transférant un objet mémoire réel et borné,
avec droits vérifiés.

### Étape 5 — réseau, firewall et connectivité

1. Définir les opérations ABI séparément pour interfaces, liens, adresse,
   résolution, connexions, métriques et configuration radio. Distinguer plan
   de contrôle (policy) et plan de données.
2. Concevoir d’abord le service réseau BAURA et son interface vers les drivers.
   Choisir un backend matériel précis et documenter PCI, DMA/IOMMU, IRQ,
   firmware et capacités nécessaires.
3. Définir le firewall comme décision de flux (direction, protocole, identité,
   ports, état, action) appliquée dans le service qui voit effectivement les
   flux. Le noyau n’est pas appelé « firewall » sans filtre dédié intégré.
4. Évaluer Go pour le service protocolaire utilisateur après validation du
   runtime et du binding IPC ; garder driver et opérations sensibles dans des
   composants à privilèges minimaux.
5. Remplacer ou encapsuler le bus de callbacks local par des événements IPC
   BAURA versionnés lorsqu’une frontière de processus est franchie.
6. Ajouter tests de parsing, règles, états de connexion, charges hostiles,
   pertes/reconnexions et tests d’intégration avec backend simulé puis NIC réel.

**Sortie :** connectivité démontrée via un adaptateur identifié ; règles de
firewall appliquées et vérifiables sur le chemin des paquets.

### Étape 6 — matrice d’alignement OS complète

Pour chaque composant Core, BOOT, GUI, communication, driver et application :

1. relever les API publiques utilisées ;
2. les rattacher à un identifiant d’opération BAURA ou justifier le contrat
   interne non exposé ;
3. relever le transport/back-end requis, les droits, l’owner de politique et
   les erreurs ;
4. éliminer les appels direct seL4 hors adaptateurs, les pointeurs dans les
   messages et les codes de succès sans effet réalisé ;
5. ajouter checks C/Rust/Go interop depuis les mêmes vecteurs de conformité.

**Sortie :** aucun consommateur système ne dépend d’une API privée implicite ;
les écarts restants sont documentés et bloquent explicitement l’opération.

L'inventaire matériel et pilotes est décrit séparément dans
[`DEVICE_DRIVER_STATUS.md`](./DEVICE_DRIVER_STATUS.md). Les callbacks PCI
internes du Core ne sont pas des opérations ABI et aucun droit matériel seL4
n'est actuellement relié au gestionnaire.

## Ordre et première tranche

Ordre obligatoire : étape 0, puis 1, puis 2 avant les services multi-tâches.
Les travaux Rust sécurité commencent sur la frontière définie à l’étape 3 ;
allocation et réseau s’appuient sur le transport et les droits stabilisés.

Première tranche commencée avec cette roadmap :

1. inclure le crate de gestion de drivers aux tests Rust ;
2. supprimer les fonctions L4 factices qui retournaient toujours des identifiants
   de tâches/adressages comme s’ils étaient réels ;
3. exiger un backend plateforme versionné pour démarrer/observer/terminer un
   driver, retourner une erreur si absent, et tester ce contrat ;
4. ne pas appeler cette implémentation un backend seL4 tant qu’aucun adaptateur
   n’est lié à l’API générée de la configuration sélectionnée.

Le contrat du crate driver a été remplacé par ce backend injectable et une
couverture de tests Rust a été ajoutée au Makefile. Ces tests sont écrits, mais
n’ont pas encore été exécutés dans l’environnement de travail ; le code ne
doit donc pas être considéré comme validé par build.
