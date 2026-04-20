NOM: BERTHE  
PRENOM: Remi  
NOM: DURAND  
PRENOM: Geoffrey

# TP3 OS User - Interpréteur BICEPS v3 (BEUIP)

## Description
Ce projet est la version finale de l'interpréteur de commandes **biceps**. Il intègre un client/serveur P2P basé sur le protocole **BEUIP**. Le programme utilise le multi-threading (`pthread`) pour faire cohabiter l'invite de commande locale, le serveur de messagerie UDP et le serveur de partage de fichiers TCP au sein du même espace mémoire partagé.

## Structure du code
* `biceps.c` : Boucle principale (`readline`), gestion de l'historique et aiguillage des commandes (`beuip start`, `beuip message`, etc.). Purge totale de la mémoire à la fermeture (`CTRL+D`).
* `gescom.c` / `gescom.h` : Parseur de commandes, allocation dynamique des mots, et exécutions via `fork/exec`.
* `servbeuip.c` : Cœur réseau contenant les deux threads serveurs (UDP pour la messagerie/présence, TCP pour le partage de fichiers bonus). L'accès à la liste chaînée est protégé par un `pthread_mutex_t`.
* `creme.c` : Fonctions clientes réseaux (envoi UDP direct, et connexions TCP pour `get` et `ls`). L'affichage de la liste est formaté strictement selon les consignes.
* `beuip_shared.h` : En-tête global. Contient le `#define BCAST_IP "192.168.88.255"`, la structure de la liste chaînée `struct elt` et la déclaration des variables partagées.

## Fonctionnalités implémentées
* [x] **Étape 1 & 2 :** Serveur threadé et liste chaînée avec insertion triée (alphabétique).
* [x] **Étape 3 (Bonus) :** Partage de fichiers en TCP avec commandes `beuip ls` et `beuip get`.
* [x] **Exigences d'évaluation :**
  * Compilation stricte validée avec `-Wall -Werror`.
  * Gestion complète de la mémoire (0 fuites avec Valgrind).
  * Nettoyage automatique du repo via `make clean` et `.gitignore`.