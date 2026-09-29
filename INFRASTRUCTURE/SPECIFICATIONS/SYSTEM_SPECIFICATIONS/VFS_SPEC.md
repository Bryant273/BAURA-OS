### SYSTÈME DE CHEMINS BAURA OS

BAURA OS utilise une syntaxe de chemin native, uniforme et indépendante des conventions propres à Windows ou Linux.

#### 1. Syntaxe native

Le format officiel des chemins BAURA est :

```text
PC/<LECTEUR>:/<CHEMIN>
```

Exemple principal :

```text
PC/C:/Ahmed/Documents
```

Exemples complets :

```text
PC/C:/
PC/C:/Ahmed/
PC/C:/Ahmed/Documents/
PC/C:/Ahmed/Documents/BAURA/
PC/C:/Ahmed/Documents/BAURA/cahier.txt
```

#### 2. Gestion des volumes

Chaque volume est identifié directement après `PC/` :

```text
PC/C:/...
PC/D:/...
PC/E:/...
PC/F:/...
```

Exemples :

```text
PC/D:/Films/Movie.mp4
PC/E:/Backup/2026/
PC/F:/Projets/INNOVKORP/
```

Les supports amovibles peuvent également être représentés par leur identifiant de volume :

```text
PC/USB:/Documents/
PC/USB_01:/Photos/
```

#### 3. Architecture logique

Le préfixe `PC/` représente l'espace de stockage logique de la machine.

Un chemin tel que :

```text
PC/C:/Ahmed/Documents/BAURA/cahier.txt
```

est interprété selon la hiérarchie :

```text
PC
 ↓
C:
 ↓
Ahmed
 ↓
Documents
 ↓
BAURA
 ↓
cahier.txt
```

Cette représentation est indépendante de l'implémentation physique du stockage.

#### 4. BAURA VFS

Le BAURA VFS constitue la couche chargée de résoudre les chemins logiques BAURA vers les volumes, systèmes de fichiers et périphériques physiques.

```text
Application
      ↓
BAURA Path API
      ↓
BAURA VFS
      ↓
Volume Manager
      ↓
Filesystem Driver
      ↓
Block Layer
      ↓
Storage Driver
      ↓
Hardware
```

Le chemin logique BAURA ne doit donc pas dépendre directement du système de fichiers utilisé.

Par exemple :

```text
PC/C:/Ahmed/Documents/
```

peut être situé sur :

```text
BAURAFS
NTFS
exFAT
FAT32
ext4
Btrfs
XFS
```

Le VFS assure l'abstraction nécessaire.

#### 5. Compatibilité Windows

Les applications Windows peuvent conserver leurs chemins natifs :

```text
C:\Users\Ahmed\Documents\Projet
```

Le Translation Engine les convertit vers la représentation logique BAURA :

```text
PC/C:/Ahmed/Documents/Projet
```

Architecture :

```text
Windows Path
      ↓
Windows Filesystem Translation
      ↓
BAURA Path Translation
      ↓
PC/C:/Ahmed/Documents/Projet
      ↓
BAURA VFS
```

#### 6. Compatibilité Linux

Les applications Linux peuvent conserver leurs chemins POSIX :

```text
/home/ahmed/Documents/Projet
```

BAURA assure leur correspondance vers son espace de chemins logique :

```text
PC/C:/Ahmed/Documents/Projet
```

Architecture :

```text
Linux/POSIX Path
      ↓
Linux Filesystem Translation
      ↓
BAURA Path Translation
      ↓
PC/C:/Ahmed/Documents/Projet
      ↓
BAURA VFS
```

#### 7. Intégration au Shell BAURA

Le Shell BAURA utilise nativement cette syntaxe.

Exemples :

```text
list PC/C:/Ahmed/Documents
```

```text
change-directory PC/C:/Ahmed/Documents/BAURA
```

```text
read PC/C:/Ahmed/Documents/BAURA/cahier.txt
```

```text
copy PC/C:/Ahmed/Documents/test.txt PC/D:/Backup/test.txt
```

```text
remove PC/C:/Ahmed/Documents/test.txt
```

#### 8. Intégration graphique

Le gestionnaire de fichiers BAURA affiche également les chemins selon la syntaxe native :

```text
PC/C:/Ahmed/Documents/BAURA/
```

L'utilisateur peut naviguer entre :

```text
PC/C:/
PC/D:/
PC/E:/
PC/USB:/
```

sans avoir à connaître l'implémentation physique du système de fichiers.

#### 9. Principe architectural

La syntaxe :

```text
PC/C:/...
```

constitue la **représentation canonique des chemins BAURA OS**.

Elle doit être prise en charge par :

- BAURA VFS
- BAURAFS
- Path API
- File API
- ABI
- SDK
- Shell BAURA
- Gestionnaire de fichiers
- Système de permissions
- Translation Engine
- Windows Compatibility Layer
- Linux Compatibility Layer
- Application Bridge
- outils système
- documentation développeur

Le principe fondamental est :

```text
UN CHEMIN LOGIQUE BAURA
          ↓
BAURA VFS
          ↓
N'IMPORTE QUEL FILESYSTEM COMPATIBLE
          ↓
STOCKAGE PHYSIQUE
```

BAURA OS possède ainsi une **identité propre de gestion des chemins**, tout en conservant la capacité d'interagir avec les environnements Windows et Linux.