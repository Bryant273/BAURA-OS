# Spécification Officielle du Format .AESB — BAURA OS
## Application Exécutable Sur BAURA (AESB)

> **Document de Référence :** `AESB-SPEC-001`  
> **Auteur :** Ahmed Silué (SDA) & Gemini (Antigravity)  
> **Date :** 16 Novembre 2026  
> **Statut :** Spécification Approuvée & Active  
> **Extension de fichier :** `.aesb`

---

## 1. Vision et Objectifs du Format .AESB

Dans **BAURA OS**, les applications ne sont pas de simples binaires bruts sans contexte, ni des scripts dépendants de runtimes lourds. Une application BAURA est encapsulée sous le format standardisé **`.aesb`** (*Application Exécutable Sur BAURA*).

Le format `.aesb` répond à 5 impératifs majeurs :
1. **Zéro hardcoding d'interface** : Le paquet contient la description déclarative de l'arbre de widgets BUI et des contraintes de mise en page récursive (2 passes).
2. **Isolation par les capacités seL4** : Le manifeste déclare strictement les droits demandés (VFS `PC/<LECTEUR>:/...`, réseau, IPC, affichage), appliqués par le noyau seL4 et BAURA Core au chargement.
3. **Conteneur unifié & auto-suffisant** : Métadonnées, code exécutable Ring 3 (ELF x86_64), ressources graphiques (icônes vectorielles BGF, sprites) et signatures cryptographiques sont regroupés en un fichier unique.
4. **Intégration transparente au VFS** : Localisées typiquement dans `PC/C:/Applications/*.aesb` ou `PC/SYSTEM:/apps/*.aesb`.
5. **Gestion du cycle de vie BUI** : Raccordement automatique aux hooks d'application (`init`, `build_ui`, `on_event`, `destroy`).

---

## 2. Structure Binaire d'un Fichier .AESB

Un fichier `.aesb` est aligné sur 64 octets et se décompose en 4 blocs principaux :

```
+-------------------------------------------------------------+
| 1. En-tête Global AESB (baura_aesb_header_t - 256 octets)   |
|    - Magic: 'AESB' (0x42534541)                             |
|    - Version, Architecture (x86_64, aarch64, riscv64)       |
|    - Manifeste Application (AppID, Nom, Version, Auteur)    |
|    - Masque de permissions de sécurité seL4                |
|    - Table des sections (Offsets & Tailles)                 |
+-------------------------------------------------------------+
| 2. Section .MANIFEST (Description BUI & Rôle de surface)    |
|    - Rôle (Window, Panel, Popup, Overlay, Service)          |
|    - Dimensions par défaut & min/max en dp                  |
|    - Déclaration de l'arbre UI initial                      |
+-------------------------------------------------------------+
| 3. Section .CODE (Exécutable binaire ELF x86_64 Ring 3)     |
|    - Segments de code (.text, .rodata, .data, .bss)         |
|    - Point d'entrée utilisateur (Entry Point)               |
+-------------------------------------------------------------+
| 4. Section .ASSETS (Ressources & Données de l'app)          |
|    - Icônes vectorielles (paths SVG / catalogue BGF)        |
|    - Textes et chaînes i18n localisées                      |
+-------------------------------------------------------------+
| 5. Section .SIGNATURE (Sécurité & Autorité BAURA)           |
|    - Hachage SHA-256 et signature Ed25519                   |
+-------------------------------------------------------------+
```

---

## 3. Définition des Structures C

```c
#define BAURA_AESB_MAGIC 0x42534541 /* "AESB" en Little Endian */
#define BAURA_AESB_VERSION 1

/* Permissions système demandées par l'application .aesb */
typedef enum {
    AESB_PERM_NONE       = 0,
    AESB_PERM_VFS_READ   = (1 << 0), /* Lecture fichiers PC/<LECTEUR>:/... */
    AESB_PERM_VFS_WRITE  = (1 << 1), /* Écriture fichiers */
    AESB_PERM_GUI_SURFACE= (1 << 2), /* Création de surfaces d'affichage BDS */
    AESB_PERM_INPUT_EVT  = (1 << 3), /* Réception clavier et souris */
    AESB_PERM_IPC_BUS    = (1 << 4), /* Communication sur le Message Bus */
    AESB_PERM_NETWORK    = (1 << 5), /* Accès pile réseau TCP/IP */
    AESB_PERM_DEVICE_RAW = (1 << 6), /* Accès matériel direct (pilotes uniquement) */
    AESB_PERM_SYS_ADMIN  = (1 << 7)  /* Élévation administrateur */
} baura_aesb_permission_flags_t;

/* Rôle de l'application dans l'environnement graphique */
typedef enum {
    AESB_ROLE_DESKTOP_APP = 0, /* Fenêtre standard (ex: Fichiers, Terminal) */
    AESB_ROLE_SHELL_PANEL = 1, /* Composant Shell (ex: Split-Dock, TopBar) */
    AESB_ROLE_POPUP_MENU  = 2, /* Menu volant transitoire (ex: BStart) */
    AESB_ROLE_BACKGROUND  = 3  /* Service de fond / Démon système */
} baura_aesb_role_t;

/* En-tête officiel du conteneur .AESB */
typedef struct {
    uint32_t magic;             /* 'AESB' */
    uint16_t version;           /* 1 */
    uint16_t arch;              /* 1 = x86_64, 2 = aarch64, 3 = riscv64 */
    
    char     app_id[64];        /* ex: "org.baura.apps.files" */
    char     name[48];          /* ex: "Fichiers" */
    char     version_str[16];   /* ex: "1.0.0" */
    char     author[48];        /* ex: "Ahmed Silué" */
    char     icon_name[32];     /* ex: "folder" */

    uint32_t role;              /* baura_aesb_role_t */
    uint32_t permissions;       /* Masque baura_aesb_permission_flags_t */

    /* Géométrie par défaut de l'interface en dp */
    int32_t  default_width_dp;
    int32_t  default_height_dp;
    int32_t  min_width_dp;
    int32_t  min_height_dp;

    /* Offsets et tailles des sections */
    uint32_t manifest_offset;
    uint32_t manifest_size;
    uint32_t code_offset;
    uint32_t code_size;
    uint32_t assets_offset;
    uint32_t assets_size;
    uint32_t signature_offset;
    uint32_t signature_size;

    uint32_t entry_point_offset; /* Point d'entrée dans la section .code */
    uint32_t checksum_crc32;
    uint8_t  reserved[32];
} __attribute__((packed)) baura_aesb_header_t;
```

---

## 4. Cycle de Vie d'une Application .AESB

1. **Localisation :** L'utilisateur clique sur une tuile du dock ou double-clique sur un fichier `.aesb` dans l'explorateur `Fichiers` (`PC/C:/Applications/terminal.aesb`).
2. **Vérification d'Autorité :** Le chargeur AESB lit l'en-tête, vérifie le checksum CRC32/Ed25519 et soumet les permissions à l'autorité de sécurité (`baura_security`).
3. **Allocation d'Espace seL4 :** Un CSpace et un VSpace isolés sont instanciés pour le processus.
4. **Mise en Place BUI :** La section manifest est injectée dans le compositeur BDS pour allouer la surface d'affichage avec le rôle déclaré.
5. **Exécution :** Le point d'entrée utilisateur est invoqué, se branchant sur la boucle d'événements du SDK BUI (`bui_app_launch`).

---

## 5. Applications .AESB Officielles Fournies de Base

| Application | Identifiant App | Rôle | Droits Requis |
|---|---|---|---|
| **`fichiers.aesb`** | `org.baura.apps.files` | Window | `VFS_READ`, `VFS_WRITE`, `GUI_SURFACE`, `INPUT_EVT` |
| **`terminal.aesb`** | `org.baura.apps.terminal` | Window | `VFS_READ`, `VFS_WRITE`, `GUI_SURFACE`, `INPUT_EVT`, `IPC_BUS` |
| **`moniteur.aesb`** | `org.baura.apps.monitor` | Window | `GUI_SURFACE`, `INPUT_EVT`, `SYS_ADMIN` |
| **`parametres.aesb`**| `org.baura.apps.settings`| Window | `VFS_READ`, `VFS_WRITE`, `GUI_SURFACE`, `IPC_BUS` |
| **`houb.aesb`** | `org.baura.apps.houb` | Window | `GUI_SURFACE`, `INPUT_EVT`, `NETWORK` |
| **`splitdock.aesb`**| `org.baura.shell.dock` | Shell Panel | `GUI_SURFACE`, `INPUT_EVT`, `IPC_BUS` |
| **`bstart.aesb`** | `org.baura.shell.bstart`| Popup Menu | `GUI_SURFACE`, `INPUT_EVT`, `IPC_BUS` |
