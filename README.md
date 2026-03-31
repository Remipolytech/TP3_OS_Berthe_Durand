# TP2 OS User - Implémentation du Protocole BEUIP et intégration à BICEPS

**Auteur :** Rémi Berthe
**Environnement :** macOS (Zsh) / POSIX / Réseau `TPOSUSER`

## Introduction

L'objectif de ce second TP de Système et Réseau (OS User) était de concevoir de A à Z un protocole de communication P2P nommé **BEUIP** (BEUI over IP), en s'inspirant du fonctionnement historique de NetBEUI. 

Le but final était d'encapsuler toute cette logique réseau dans une bibliothèque statique en C (`creme` ) pour l'intégrer sous forme de commandes internes interactives dans **biceps**, l'interpréteur de commandes que nous avons développé lors du TP1. L'ensemble a été testé et validé en conditions réelles sur le réseau local de la salle de TP (`192.168.88.0`).

---

## Étape 1 : Des datagrammes avec accusé de réception

**Objectif :** Mettre en place une communication client/serveur de base en UDP avec un mécanisme d'accusé de réception (AR).

* **Réalisation :** Modification du code de base `servudp.c` pour qu'il renvoie systématiquement la chaîne "Bien reçu 5/5 !".
* **Méthode :** Utilisation des informations réseau de l'expéditeur extraites de la structure `sockaddr_in` (remplie par `recvfrom()` ) pour cibler la réponse via `sendto()`. *(Note : le flag `MSG_CONFIRM` a été ignoré via compilation conditionnelle pour garantir la compatibilité sur macOS)*.

---

## Étape 2 : Création du Protocole BEUIP (Réseau TPOSUSER)

**Objectif :** Créer un serveur autonome capable de s'annoncer sur le réseau, de maintenir une table d'utilisateurs (IP/Pseudo), et de gérer une messagerie multi-clients.

***Réalisation :** Développement de `servbeuip.c` qui écoute en tâche de fond sur le port 9998.
***Format des messages :** Code d'action (1 octet) + En-tête de validation "BEUIP" (5 octets) + Corps du message (Pseudo ou texte).
* **Prise en charge des codes du protocole :** * `1` : Broadcast de présentation (sur `192.168.88.255` ).
  * `2` : Accusé de réception.
  * `3` : Demande de liste.
  * `4` et `9` : Envoi et réception de Message privé.
  * `5` : Message à tout le monde.
  * `0` : Signal de déconnexion pour mise à jour des tables.
***Sécurisation :** Le serveur vérifie rigoureusement que les commandes d'interface (liste, mp, all) proviennent bien de la boucle locale `127.0.0.1` pour éviter les usurpations d'identité.

---

## Étape 3 : Intégration finale dans le shell BICEPS

**Objectif :** Rendre notre shell autonome en y intégrant les fonctionnalités réseau via une librairie dédiée.

***Création de la librairie `creme`** (Commandes Rapides pour l'Envoi de Messages Evolués) : Extraction de la logique d'envoi UDP locale vers `creme.c` et `creme.h`.
* **Commande `beuip start/stop` :**
  * Le shell utilise `fork()` et `execvp()` pour lancer le serveur en tâche de fond et sauvegarde son PID.
  * La commande `stop` envoie le signal `SIGUSR1` au processus fils. Le serveur intercepte ce signal, diffuse un message de déconnexion (code '0') sur le réseau, puis s'arrête proprement.
* **Commande `mess` :** Elle exploite la librairie `creme` pour envoyer les requêtes de l'utilisateur (liste, mp, all) directement au serveur local.

### Validation en conditions réelles

Voici le log d'exécution confirmant le bon fonctionnement du protocole avec les autres étudiants connectés au réseau `TPOSUSER` :

```text
(base) remiberthe@Mac TP2 % ./biceps 
remiberthe@Mac.TPOSUSER$ beuip start Remi 
Serveur BEUIP lancé en tâche de fond (PID: 49497). 
Serveur BEUIP (Remi) en écoute sur le port 9998... 
[+] dede a rejoint le réseau (192.168.88.137) 
[+] alice a rejoint le réseau (192.168.88.232) 
[+]  a rejoint le réseau (192.168.88.236) 

remiberthe@Mac.TPOSUSER$ mess liste 
=== Liste des utilisateurs (3) === 
  - alice (192.168.88.137) 
  - alice (192.168.88.232) 
  -  (192.168.88.236) 
================================= 

remiberthe@Mac.TPOSUSER$ mess all "Salut la salle !" 
[->] Message envoyé à 3 personne(s). 

remiberthe@Mac.TPOSUSER$ mess mp alice "Coucou ?" 
[->] Message privé envoyé à alice. 

remiberthe@Mac.TPOSUSER$ beuip stop 
Envoi du signal d'arrêt au serveur BEUIP... 
[!] Signal reçu. Arrêt du serveur BEUIP... 
Serveur BEUIP arrêté. 

remiberthe@Mac.TPOSUSER$ exit 
Bye ! Merci d'avoir utilisé biceps.