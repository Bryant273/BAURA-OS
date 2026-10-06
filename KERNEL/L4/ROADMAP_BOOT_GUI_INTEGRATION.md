# Feuille de route — démarrage BAURA et intégration du GUI

## Objet et périmètre

Cette feuille de route organise le raccordement du démarrage x86_64 seL4,
BAURA Core et du bureau BGE, ainsi que l’intégration d’une bibliothèque
d’icônes. Elle complète
[`ROADMAP_MICROKERNEL_BAURA.md`](./ROADMAP_MICROKERNEL_BAURA.md) sans modifier
la roadmap historique
[`BAURA OS — Roadmap du noyau et du système de boot.md`](../../BAURA%20OS%20%E2%80%94%20Roadmap%20du%20noyau%20et%20du%20syst%C3%A8me%20de%20boot.md).

La cible de travail provisoire est le PC x86_64 et la configuration seL4
`pc99` actuellement inscrite dans le Makefile. Ce choix reste à confirmer pour
la plateforme matérielle et le chemin de lancement retenus.

## Règle d’architecture : qui fait quoi ?

- **seL4 fournit et protège les mécanismes** : tâches/threads, espaces
  d’adressage, capacités, IPC, interruptions et accès autorisés aux ressources
  matérielles.
- **L’adaptateur de plateforme BAURA** est le code spécifique à la version et
  à la configuration seL4. Il interprète le vrai `BootInfo`, vérifie les
  capacités reçues et prépare des ressources BAURA sûres : console, création
  de tâches, autorité, mémoire d’affichage et entrée lorsqu’elles sont
  réellement disponibles.
- **BAURA Core applique la politique et supervise les services** : il reçoit
  des callbacks/handles validés, initialise ses gestionnaires et demande le
  démarrage du GUI. Il ne fabrique pas de capacité seL4 et ne mappe pas
  directement une adresse physique. Son gestionnaire de processus ne crée pas
  de tâche sans backend concret.
- **BGE et le runtime GUI dessinent et traitent les événements** sur des
  ressources fournies. Ils ne découvrent pas le matériel et ne contournent pas
  les contrôles du noyau.

Ainsi, le lien logique de service se fait bien **via BAURA Core**, mais la
jonction de bas niveau avec seL4 doit être implémentée par l’adaptateur de
plateforme utilisé par la tâche initiale. « Directement auprès du Core » ne
signifie pas que le Core remplace seL4 ou peut s’accorder des droits matériels.

## État de départ vérifié dans le dépôt

- La cible de démarrage par défaut (`BOOT/boot.asm`, `grub.cfg`) est encore le
  bring-up Multiboot 32 bits ; elle ne lance pas la cible seL4 x86_64.
- Le Makefile configure seL4 avec `PLATFORM=pc99`, `KernelSel4Arch=x86_64` et
  `KernelPrinting=ON`, et construit le noyau séparément. Il ne construit pas
  encore l’image de tâche initiale BAURA ni une image de démarrage combinée.
- Le contrat Core garde le BootInfo opaque et vérifie l’entrée via un
  validateur fourni par la plateforme. Aucun validateur/adaptateur seL4
  concret n’est actuellement lié.
- Un hook GUI optionnel, compatible avec l’ancienne taille de structure, a été
  ajouté au contrat de plateforme Core. Le pont GUI peut enregistrer le
  runtime dans le gestionnaire de services. Ce hook ne fournit pas les
  capacités ni les mappings seL4.
- Le runtime GUI accepte un framebuffer 32 bits RGBX/BGRX déjà mappé et le
  pilote PS/2 utilise des callbacks d’E/S fournis par l’appelant. Aucun code
  ne procure actuellement ce mapping, les capacités de ports ou des IRQ.
- Le démarrage actuel du runtime ne constitue pas encore un service graphique
  persistant : `start()` dessine une première image puis rend la main. Il n’y
  a pas encore de tâche GUI dédiée ni de boucle d’événements au démarrage.
- Le catalogue d’icônes contient des identifiants et des teintes, pas les
  illustrations Papirus. Le renderer dessine encore des primitives.
- Les tests hôtes ajoutés au Makefile ne sont pas déclarés réussis tant qu’ils
  n’ont pas été exécutés dans un environnement de build.

Ces constats sont ceux des fichiers du dépôt à la date de rédaction. Ils ne
prouvent ni un démarrage sur matériel, ni la disponibilité d’un framebuffer
après le transfert de contrôle au noyau.

## Parcours de livraison

### Étape 0 — établir un état de référence reproductible

1. Exécuter et corriger les suites disponibles : `make test-core`,
   `make test-core-x86_64`, `make test-boot` et `make test-gui`.
2. Vérifier le build du noyau seL4 `pc99` x86_64 avec la version, les options
   générées et les outils réellement installés ; archiver les commandes et
   résultats.
3. Vérifier séparément le build x86_64 du Core. Ne pas présenter ces deux
   builds séparés comme une image OS intégrée.

**Sortie attendue :** résultats reproductibles et liste précise des écarts
entre interfaces hôtes, noyau compilé et démarrage effectivement réalisable.

### Étape 1 — figer le contrat de boot avant d’écrire l’adaptateur

1. Choisir l’environnement initial : émulateur de référence, puis matériel
   cible ; décider explicitement UEFI/BIOS et du rôle exact de GRUB ou de tout
   autre chargeur.
2. Pour cette configuration précise, confirmer dans la documentation et dans
   le build :
   - le protocole par lequel le chargeur lance seL4 ;
   - le format et le mode de chargement de la tâche utilisateur initiale ;
   - le mode d’assemblage du noyau et de l’image utilisateur ;
   - l’adresse/registre et le layout du vrai `seL4_BootInfo`, y compris les
     chunks variables ;
   - les CSpaces, capacités, frames et autres objets remis à la tâche initiale.
3. Vérifier explicitement si la configuration conserve une information GOP /
   framebuffer, qui en possède les frames et sous quelle forme ils peuvent
   être mappés dans l’espace utilisateur. Si elle ne le garantit pas, choisir
   un autre chemin d’affichage documenté ; ne pas déduire un framebuffer
   utilisable d’un ancien tag Multiboot.
4. Établir la liste minimale des capacités que l’initial task doit conserver
   et celles qu’il déléguera ensuite. Inclure seulement les droits nécessaires
   au service concerné.
5. Vérifier les contraintes des ressources x86 choisies : les ports d’E/S
   nécessitent des capacités IO Port ; les interruptions demandent la création
   et la délégation des objets IRQ/notifications appropriés.

**Sortie attendue :** une fiche de configuration vérifiable (révision seL4,
options CMake, chargeur, formats, contrat BootInfo, plan de capacités et
politique framebuffer/entrée). Toute donnée inconnue reste marquée comme
bloquante, pas remplacée par une hypothèse.

### Étape 2 — construire et démarrer la tâche initiale BAURA minimale

1. Ajouter le runtime et les bibliothèques utilisateurs nécessaires à la
   configuration seL4 retenue. Construire un exécutable ELF adapté au contrat
   confirmé ; l’archive statique Core seule ne suffit pas.
2. Implémenter l’adaptateur seL4 hors du Core portable. Il consomme les headers
   et l’API produits par le build exact, valide les capacités reçues et adapte
   les appels seL4 aux callbacks versionnés de `BauraCorePlatform`.
3. Commencer par la console de diagnostic autorisée et l’initialisation Core.
   Sur BootInfo invalide ou ressource obligatoire manquante, retourner une
   erreur explicite et ne pas afficher un état « prêt » trompeur.
4. Assembler le noyau et l’exécutable utilisateur uniquement selon le chemin
   établi à l’étape 1. Générer un artefact de boot reproductible.
5. Lancer dans l’émulateur retenu et vérifier les logs, la validation du
   BootInfo et le traitement des erreurs. Le démarrage matériel vient ensuite.

**Sortie attendue :** le chemin documenté atteint réellement une tâche
utilisateur BAURA qui initialise Core et rapporte son état. Cela ne signifie
pas encore qu’un service GUI est démarré.

### Étape 3 — rendre l’interface de ressources graphique explicite

1. Définir un contrat de ressource GUI indépendant des structures seL4 :
   framebuffer virtuellement accessible, dimensions, pitch, format, longueur,
   propriété/durée de vie du mapping, méthode de présentation/flush, et erreurs.
2. L’adaptateur vérifie les bornes, les droits d’écriture, la taille et la
   durée de vie des frames avant de les remettre au runtime GUI. Le Core ne
   reçoit pas d’adresse physique brute à convertir lui-même.
3. Choisir une stratégie d’entrée pour la première cible :
   - évaluer PS/2 si le matériel et les capacités autorisées le permettent ;
   - sinon retenir un périphérique d’entrée plus approprié à l’émulateur ou
     au matériel cible.
4. Pour PS/2, l’adaptateur implémente les callbacks au moyen des appels seL4
   autorisés. Si le pilote devient IRQ-driven, fournir explicitement les
   capacités IRQ, notifications et acknowledgements requis. Ne pas faire
   d’instructions `in`/`out` privilégiées depuis le code portable.
5. Garder séparés le backend d’affichage et le backend d’entrée afin que
   chacun puisse être testé/mocké sans matériel.

**Sortie attendue :** ressources de framebuffer et d’entrée validées par la
plateforme, injectées dans BAURA, avec tests des dimensions, erreurs,
capacité insuffisante et absence de droits.

### Étape 4 — démarrer un service GUI qui reste actif

1. Choisir la première topologie : GUI dans la tâche initiale pour un
   prototype minimal, ou tâche utilisateur dédiée si le backend de création
   de tâches est prêt. Le choix et ses limites doivent être explicites.
2. Donner à la boucle GUI un cycle de vie réel : initialisation, événements,
   rendu au besoin, attente/synchronisation adaptée, arrêt et propagation des
   erreurs. Éviter un polling sans borne qui monopoliserait le processeur.
3. Si le GUI devient une tâche séparée, implémenter d’abord le backend de
   processus seL4 et le transfert des seules capacités d’affichage/entrée
   nécessaires. BAURA Core reste le superviseur/politique ; seL4 crée et
   isole la tâche.
4. Connecter les actions de la taskbar aux services BAURA seulement lorsqu’ils
   existent, et présenter explicitement les actions indisponibles.
5. Valider par paliers : runtime avec framebuffer simulé, tâche GUI sous
   émulateur, interactions clavier/souris, puis tests sur la machine cible.

**Sortie attendue :** un bureau persistant qui reçoit des événements et
présente des frames par le chemin réel. Un simple `start()` réussi ou une
image fixe ne satisfait pas ce critère.

### Étape 5 — intégrer une bibliothèque d’icônes avec traçabilité

1. Comparer les sources candidates, en vérifiant les dépôts et les fichiers de
   licence réellement associés aux assets : Papirus, Breeze et Adwaita.
   Enregistrer dépôt, révision/tag, licence des fichiers retenus, notices,
   exigences d’attribution et état des modifications. Les résumés de licence
   fournis dans une conversation ne remplacent pas ces vérifications.
2. Tenir compte de l’impact des licences sur la distribution du système : le
   caractère privé ou local du dépôt ne supprime pas les obligations attachées
   aux assets.
3. Prototyper un petit échantillon réel avec le style BAURA. Comparer lisibilité
   aux tailles utilisées, contraste en thèmes clair/sombre, cohérence visuelle,
   couverture des icônes système et facilité de mise à jour.
4. Commencer par un set limité : dossiers/fichiers, paramètres, recherche,
   réseau, batterie, notifications, média, fenêtres, compte/verrouillage et
   installation. Ajouter les icônes d’applications selon leur besoin.
5. Préférer un traitement à la construction des SVG vers une ressource bitmap
   bornée comprise par BGE, plutôt qu’un interpréteur SVG général dans l’OS.
   Confirmer le format/pipeline par un prototype : aucun décodeur PNG ou SVG
   système n’est supposé disponible aujourd’hui.
6. Étendre le catalogue avec identifiant stable, asset, dimensions, source,
   licence et fallback BAURA original. Toute recoloration/personnalisation doit
   être documentée comme modification et préserver l’illustration d’origine.
7. Ajouter des tests de chargement, ressource manquante, limites de dimensions,
   format invalide et rendu des assets embarqués.

**Sortie attendue :** quelques icônes officielles livrées et traçables, rendues
par BGE sans dépendance à un système de fichiers Linux installé sur la cible.

## Ordre de travail et blocages

Les étapes 0 et 1 précèdent obligatoirement l’adaptateur et l’image intégrée.
L’étape 2 précède le raccordement graphique de démarrage. L’étape 3 fournit
les ressources nécessaires à l’étape 4. La sélection/licence des icônes peut
avancer en parallèle après choix du pipeline graphique, mais l’ajout d’assets
au dépôt attend la vérification de leur provenance et licence.

### Prochaine tranche concrète

1. Lancer les vérifications hôtes disponibles et relever ce qui échoue.
2. Relever la configuration exacte seL4 générée pour `pc99` x86_64 et le
   chemin d’amorçage officiellement pris en charge pour cette version.
3. Choisir l’environnement de première démonstration (QEMU ou machine) et
   obtenir le contrat explicite de chargement de l’initial task.
4. À partir de ces faits, écrire l’adaptateur minimal console + validation
   BootInfo ; ne pas ajouter de mapping graphique avant que son contrat soit
   établi.
5. En parallèle, comparer les dépôts/licences des trois bibliothèques et
   préparer un prototype d’assets hors intégration système.

## Critères de communication de l’avancement

Pour chaque jalon, séparer dans les comptes rendus :

- **Vérifié** : fichiers/configuration consultés, commande et résultat obtenus.
- **Implémenté** : code présent, et tests réellement passés ou non.
- **À décider** : choix produit ou plateforme qui reste ouvert.
- **Bloqué** : information, outil ou ressource manquante.

Ne pas confondre compilation du noyau, tests hôtes, boot de l’initial task,
service utilisateur persistant et fonctionnement graphique sur matériel : ce
sont des critères distincts.
