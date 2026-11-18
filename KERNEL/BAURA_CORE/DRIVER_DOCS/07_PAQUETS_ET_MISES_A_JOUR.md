# 07 — Paquets de pilotes, signature, retour arrière, pannes

Précise le §6 du guide. Tout ce qui suit est une **proposition** sauf mention.

## 1. Ce qui existe — [vérifié]

- Le gestionnaire de mises à jour est une machine à états **globale** :
  `BauraUpdateState` (IDLE, VALIDATING, APPLYING, ROLLING_BACK, COMPLETE, FAILED,
  ROLLBACK_FAILED), `baura_update_apply(target, verify, apply, rollback, ctx)`,
  versions `uint64`. Il n'y a **qu'un état et une version pour tout le système**,
  donc une mise à jour par pilote demande un enregistrement par pilote
  (à ajouter).
- Le VFS résout des chemins `PC/<volume>:/...` mais **aucun système de fichiers
  n'est implémenté** : pas de stockage persistant pour les paquets.

## 2. Phases réalistes

- **U0** : pilotes **compilés dans l'image** ; « mise à jour » = nouvelle image.
- **U1** : pilotes dans un **initrd/module GRUB** (`module2`), chargés au
  démarrage, vérifiés par signature.
- **U2** : magasin persistant sur un volume (requiert un système de fichiers +
  écriture sûre + A/B).

Ne pas prétendre à U2 tant que U0 et U1 ne sont pas validés sur matériel.

## 3. Manifeste

```json
{
  "schema": 1,
  "name": "virtio-blk",
  "version": "0.1.0",
  "version_packed": 4096,
  "abi": 2,
  "license": "BSD-2-Clause",
  "origin": "écrit pour BAURA ; spécification virtio 1.x",
  "match": [ {"vendor": "0x1af4", "device": "0x1042"} ],
  "requires": { "ioports": [], "irqs": 1, "bars": [4], "dma": true },
  "trust_level": 1,
  "sha256": "<hex>",
  "signature": "<base64>"
}
```

`version_packed` : `major<<32 | minor<<16 | patch` (uint64, comparable). `abi`
doit être égal à l'ABI exposée par le Core ; sinon refus. `requires` est la
liste **exhaustive** des capabilities à déléguer (`05_…`).

## 4. Signature — [à décider dans 08]

Proposition : **Ed25519** pour la signature, **SHA-256** pour le condensat,
bibliothèque auditée et petite. Clé de publication hors ligne ; **rotation** :
le Core embarque une liste de clés de confiance et un numéro de révocation ;
**révocation** : liste signée de condensats/versions interdits (empêche aussi le
retour vers une version vulnérable : « anti-rollback » = version minimale par
pilote, stockée hors de la portée des pilotes).

## 5. Retour arrière

Garder N-1 : installer à côté, basculer **seulement** après un démarrage réussi
(`start` OK + test de santé), sinon revenir à N-1. En U0/U1 le retour arrière
est « démarrer l'ancienne image/module » (entrée GRUB précédente) : c'est
pourquoi le menu GRUB visible (`01_…` §6) compte.

## 6. Politique de panne — valeurs par défaut **ajustables**

3 plantages en 60 s → quarantaine du pilote (`BAURA_BIND_QUARANTINED`),
redémarrage avec délai croissant (1 s, 2 s, 4 s…) tant que le seuil n'est pas
atteint ; un pilote du chemin de démarrage (disque système) en quarantaine
déclenche un repli vers la version N-1. Ces nombres n'ont **aucune base
empirique** : à mesurer. Le moteur de reprise du Core (un handler par code de
faute, capacité 16) doit porter cette logique.

## 7. Ce qu'on ne signe/vérifie pas encore

Aucune vérification cryptographique n'existe dans le Core (le gestionnaire de
mises à jour délègue `verify` à un callback). Tant que ce callback n'est pas
écrit et testé (vecteurs de test officiels Ed25519/SHA-256), **ne rien charger
depuis un support externe**.
