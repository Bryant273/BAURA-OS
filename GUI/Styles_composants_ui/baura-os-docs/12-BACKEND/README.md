# 12 — Backend fonctionnel : Rust + interface C + C++

Jusqu'au document 11, **tout le code de ce dossier était du C++** (dessin pur, état fourni par l'appelant). Ce document ajoute le **backend** : la logique et les données que l'interface affiche, écrite en **Rust** (sûreté mémoire, tests rapides, même langage que les crates existants du dépôt : `KERNEL`, `STORAGE`, `VFS`, `SECURITY`, `POWER_MANAGER`, `DRIVER_MANAGER`), exposée au C++ par une **interface C stable**. Le C++ reste responsable du dessin et de la saisie ; le Rust possède l'état.

```
        C++ (BGE, kit baura_ui, écrans)                         Rust (workspace baura-ui-services)
 ┌─────────────────────────────────────────┐   ABI C   ┌────────────────────────────────────────────────┐
 │ baura_services.hpp  (RAII, copie les    │──────────▶│ baura-ffi  (staticlib libbaura_ui_services.a)   │
 │   chaînes, aucun pointeur Rust exposé)  │  snapshot │   ├─ baura-prefs   préférences typées, versionnées│
 │ BauraShell::*, BauraMonitor::*, …       │  + ordres │   ├─ baura-sysmon  mesures, historique, impact     │
 │   lisent une VUE, dessinent, renvoient  │◀──────────│   ├─ baura-icons   résolution id → icône           │
 │   des commandes (clic, réglage, kill)   │           │   ├─ baura-shell   barre, Démarrer, recherche, notifs │
 └─────────────────────────────────────────┘           │   └─ baura-i18n    catalogues, pluriels, RTL       │
                                                       └────────────────────────────────────────────────┘
```
Principe « **snapshot + commandes** » : le C++ n'a jamais de référence vers un objet Rust ; il demande une vue (`baura_taskbar_refresh`, `baura_monitor_group`…), la dessine, puis envoie une commande (`baura_taskbar_click`, `baura_prefs_set_layout`, `baura_monitor_terminate`). Le compteur `baura_prefs_version` dit à l'interface *quand* redessiner.

## 1. Contenu
| Élément | Chemin |
|---|---|
| Workspace Cargo (sans dépendance externe) | `baura-ui-services/Cargo.toml` |
| 6 crates | `baura-ui-services/crates/baura-{prefs,sysmon,icons,shell,i18n,ffi}` |
| En-tête C écrit à la main | `baura-ui-services/include/baura_ui_services.h` |
| Enveloppe C++ RAII | `baura-ui-services/include/baura_services.hpp` |
| Test d'intégration C++ → Rust (`-Wall -Wextra -Werror`) | `baura-ui-services/tests/test_backend.cpp` |
| Catalogues de traduction (fr, en, ar) | `crates/baura-i18n/catalogs/*.txt` |

## 2. Ce que fait chaque crate
| Crate | Responsabilité | Détails vérifiés par les tests |
|---|---|---|
| `baura-prefs` | Disposition du bureau (**barre coupée en deux = défaut**, pastille flottante, pleine largeur), thème (auto/clair/sombre/contraste élevé), accent, **style d'icônes** (BAURA / d'origine), **densité**, langue, direction, échelle de texte, réduire mouvement/transparence, applications épinglées | format texte `clé=valeur` versionné ; lecture **tolérante** (clés inconnues ignorées, valeurs invalides signalées et remplacées par le défaut) ; `version` n'augmente qu'en cas de vrai changement ; RTL déduit de la langue |
| `baura-sysmon` | Source de mesures (`Synthetic` déterministe · `LinuxProc` lit `/proc` sur l'hôte), historique en anneau, **regroupement par application**, classement d'**impact** (Faible/Moyen/Élevé), **terminaison** d'un groupe avec protection « Système » | déterminisme, bornes, regroupement, tri, seuils, anneau, terminaison, fumée sur le vrai `/proc` |
| `baura-icons` | Résolution `id → icône` (exact → alias → application connue → catégorie → générique) ; **tables générées** par le même script que le C++ | toute cible de table existe ; ids uniques ; noms affichés |
| `baura-shell` | **Barre des tâches** (épinglées + fenêtres ouvertes, badge, attention), **décision du clic** (lancer / focaliser / réduire / restaurer / parcourir), **menu Démarrer** (épinglées, récentes, par catégorie), **recherche classée** (préfixe > début de mot > sous-chaîne > sous-séquence, sans accents, boost des récents), **notifications** (file bornée à 50, groupes, « ne pas déranger », urgentes) | logique de clic sans interface ; classement ; limites |
| `baura-i18n` | Catalogues, repli `fr-CA → fr → en → clé`, **pluriels** (fr, en, **arabe à 6 formes**, ja/zh/ko), direction | complétude des catalogues (clé manquante = test rouge) |
| `baura-ffi` | ABI C : un objet opaque `BauraServices`, chaînes copiées dans un tampon appelant, retour = longueur nécessaire | parcours complet *via l'ABI C* dans un test Rust ; puis test C++ |

## 3. Qui sert quoi : couverture du BGE documenté
| Écran / composant (doc) | Service backend | État |
|---|---|---|
| Barre des tâches : épinglées, ouvertes, badge, clic (07) | `baura-shell::taskbar/click` + `prefs.taskbar_pins` | ✅ implémenté + testé |
| Menu Démarrer : épinglées/récentes/toutes (07) | `baura-shell::start_view` | ✅ |
| Recherche : applications (07) | `baura-shell::search` | ✅ ; sources **réglages** et **fichiers** : types prévus (`Kind::Setting/File`), à alimenter par l'index VFS |
| Notifications / centre d'actions (07) | `baura-shell::Notifications` | ✅ ; bascules rapides (Wi-Fi, Bluetooth, luminosité) : **à écrire** (services réseau/affichage) |
| Paramètres › Apparence, Bureau, Langue, Accessibilité (07, 08) | `baura-prefs`, `baura-i18n` | ✅ ; **persistance** sur disque : sérialisation prête, branchement sur `VFS` à faire |
| Icônes partout (09, 11) | `baura-icons` + tables générées | ✅ |
| **Moniteur** (13) | `baura-sysmon` | ✅ avec source synthétique et `/proc` ; source **BAURA Core** (compteurs noyau) : à écrire |
| Fichiers (07) | crate `VFS` **existante** du dépôt | ⬜ à raccorder (liste de dossier → lignes du détail) |
| Sécurité / UAC / mot de passe (07) | crate `SECURITY` **existante** + service d'élévation | ⬜ à spécifier avec le noyau |
| Énergie (07) | `POWER_MANAGER` **existant** (profils, batterie) | ⬜ à raccorder |
| Réseau, Son, Mise à jour, Boutique, HOUB, BMedia, Terminal | — | ⬜ services à créer (chantiers séparés ; voir `10` P8) |
| Fenêtres (05) | gestionnaire de fenêtres C++ du BGE → `baura_window_open/close/set_badge` | ✅ interface ; câblage dans `BGEWindowManager` à faire |
| Accessibilité : arbre, focus (08) | reste en **C++** (lié au dessin) ; préférences côté Rust | ✅ partiel |

Légende : ✅ fait et testé ici · ⬜ conçu (interface connue) mais **non écrit**. Cette table est volontairement franche : le backend couvre **toute la coque et le Moniteur**, pas encore chaque application.

## 4. Construire et tester
```bash
cd 12-BACKEND/baura-ui-services
cargo test                      # 34 tests Rust (sans réseau, sans dépendance)
cargo build --release           # target/release/libbaura_ui_services.a
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/test_backend.cpp target/release/libbaura_ui_services.a -lpthread -ldl -lm -o /tmp/tb && /tmp/tb
```
`../build_all.sh 12` fait tout cela. Le Moniteur (`13`) et les écrans l'utilisent via `baura_services.hpp`.

## 5. Vers seL4 (non fait ici)
- `baura-prefs`, `baura-shell`, `baura-i18n`, `baura-icons` n'utilisent que `std::{string, vec, collections}` : ils passent en `#![no_std]` + `alloc` avec peu de changements (retirer `std::fs`, `format!` → `alloc::format!`). `baura-sysmon::LinuxProc` est **réservé à l'hôte** de développement.
- Dans BAURA Core, le backend devient un **service utilisateur** (tâche seL4) ; l'ABI C est conservée entre l'interface et lui, ou remplacée par des messages IPC avec les mêmes structures (`BauraSnapshot`, `BauraGroup`…).
- `panic = "abort"` est déjà réglé dans le profil `release`.

## 6. Limites
- **Jamais exécuté sur seL4/QEMU** : validé sur l'hôte (Linux), comme le reste.
- La source de mesures `Synthetic` produit des courbes plausibles mais **fausses** : c'est un outil de démonstration/test, pas une mesure.
- `LinuxProc` est volontairement simple (pas de cgroups, pas de disque par processus, regroupement par *nom de processus*).
- « Terminer » dans le Moniteur **retire** le groupe du modèle ; envoyer réellement un signal à BAURA Core est une commande à ajouter côté noyau (non spécifiée dans le dépôt).
- L'ABI n'est pas encore versionnée finement (`baura_version()` renvoie 1.0) : à figer avant d'avoir des clients externes.
