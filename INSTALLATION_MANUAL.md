
================================================================================
                   MANUEL D'INSTALLATION ET DE DÉPLOIEMENT
                                BAURA OS v1.0
================================================================================

1. PRÉREQUIS MATÉRIELS
-----------------------
- Architecture: x86_64 (Intel Core / AMD Ryzen)
- RAM: 4 Go minimum (8 Go recommandés pour la virtualisation BVM)
- Stockage: 20 Go minimum sur disque SATA/NVMe
- BIOS/UEFI: Mode Legacy ou UEFI (compatible GRUB Multiboot)

2. PRÉPARATION DU SUPPORT (USB)
-------------------------------
L'image ISO de BAURA OS est conçue pour être flashée directement sur un support amovible.

Commande Linux:
  sudo dd if=baura_os.iso of=/dev/sdX bs=4M status=progress && sync
(Remplacez /dev/sdX par l'identifiant de votre clé USB)

3. PROCÉDURE DE BOOT
--------------------
1. Insérez la clé USB dans la machine cible.
2. Accédez au menu de boot (F12, F11, F8 ou Esc selon le constructeur).
3. Sélectionnez "USB Storage Device" ou "UEFI: USB Disk".
4. Au menu GRUB, sélectionnez "BAURA OS" et appuyez sur Entrée.

4. INSTALLATION SUR DISQUE
---------------------------
Une fois l'OS démarré en mode Live:
1. Lancez l'application "Installateur Graphique".
2. Sélectionnez la langue et la disposition du clavier.
3. Choisissez la partition cible (Attention: l'installation efface les données).
4. Le système installera le noyau L4, la BAURA-Lib, et l'image système.
5. Redémarrez la machine.

5. PREMIÈRE CONNEXION (SÉCURITÉ)
---------------------------------
Au premier démarrage, BAURA OS vous demandera :
- De créer un compte Administrateur (root).
- De définir une clé de chiffrement pour l'Enclave Sécurisée (AES-256).
- De configurer le réseau (WiFi/Ethernet) via le NetManager.

6. DÉPANNAGE (TROUBLESHOOTING)
------------------------------
- Écran Noir au Boot: Vérifiez que le "Secure Boot" est désactivé dans le BIOS.
- Pas de Clavier/Souris: Vérifiez que les ports USB sont en mode XHCI.
- Crash lors d'une app: Utilisez le 'Gestionnaire de Tâches' pour envoyer un SIGKILL
  ou consultez les logs via l'API 390 (BAURA_SYS_LOG_READ).

================================================================================
                    Soutien Technique : INNOV'KORP / BAURA OS
================================================================================
