#ifndef BEUIP_SHARED_H
#define BEUIP_SHARED_H

#include <pthread.h>
#include <netinet/in.h>

#define PORT 9998
#define LPSEUDO 23

// L'adresse de broadcast est centralisée ici pour être modifiée facilement
#define BCAST_IP "192.168.88.255" 

// La structure de base pour notre liste chaînée de contacts
struct elt { 
    char nom[LPSEUDO+1]; 
    char adip[16]; 
    struct elt *next; 
};

// Variables globales partagées entre notre shell et nos serveurs réseaux
extern struct elt *listeUsers;
extern pthread_mutex_t mutexUsers; // Indispensable pour éviter les conflits d'écriture/lecture

extern int sid_global;
extern int sid_tcp_global;
extern int serveur_actif;
extern char monPseudo_global[LPSEUDO+1];

// Signatures des fonctions pour la partie UDP (Messagerie)
pthread_t init_serveur_udp(char *pseudo);
void commande(char octet1, char *message, char *pseudo);
void listeElts(void);
void liberer_liste(void); // Utilisée à la fermeture pour Valgrind

// Signatures des fonctions pour la partie TCP (Fichiers)
pthread_t init_serveur_tcp(char *rep);
void demandeListe(char *pseudo);
void demandeFichier(char *pseudo, char *nomfic);

#endif