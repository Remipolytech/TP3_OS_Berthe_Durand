#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "beuip_shared.h"

// Définition des variables partagées déclarées dans beuip_shared.h
struct elt *listeUsers = NULL;
pthread_mutex_t mutexUsers = PTHREAD_MUTEX_INITIALIZER; // Mutex pour éviter que deux threads modifient la liste en même temps
int sid_global = -1, sid_tcp_global = -1, serveur_actif = 0;
char monPseudo_global[LPSEUDO+1];

// Fonction de nettoyage pour Valgrind : on parcourt et libère chaque maillon de la liste
void liberer_liste(void) {
    pthread_mutex_lock(&mutexUsers); // On verrouille pour la sécurité mémoire
    struct elt *c = listeUsers;
    while (c != NULL) {
        struct elt *tmp = c;
        c = c->next;
        free(tmp); // On libère le maillon actuel
    }
    listeUsers = NULL;
    pthread_mutex_unlock(&mutexUsers);
}

// Ajoute un utilisateur en gardant la liste triée par ordre alphabétique (Consigne 2.2)
void ajouteElt(char *pseudo, char *adip) {
    pthread_mutex_lock(&mutexUsers);
    struct elt *c = listeUsers, *p = NULL;
    
    // On vérifie si l'IP n'est pas déjà présente pour éviter les doublons
    while (c != NULL) { 
        if (strcmp(c->adip, adip) == 0) { 
            pthread_mutex_unlock(&mutexUsers); 
            return; 
        } 
        c = c->next; 
    }
    
    // Utilisation de calloc pour initialiser à zéro (évite les avertissements Valgrind sur mémoire non initialisée)
    struct elt *n = calloc(1, sizeof(struct elt)); 
    strncpy(n->nom, pseudo, LPSEUDO); 
    strncpy(n->adip, adip, 15);
    
    // Insertion triée : on cherche la position où insérer selon le pseudo
    c = listeUsers;
    while (c != NULL && strcmp(c->nom, pseudo) < 0) { p = c; c = c->next; }
    n->next = c; 
    if (p == NULL) listeUsers = n; else p->next = n;
    
    pthread_mutex_unlock(&mutexUsers);
}

// Supprime un utilisateur de la liste (utile lors de la réception du code '0')
void supprimeElt(char *adip) {
    pthread_mutex_lock(&mutexUsers);
    struct elt *c = listeUsers, *p = NULL;
    while (c != NULL) {
        if (strcmp(c->adip, adip) == 0) {
            if (p == NULL) listeUsers = c->next; else p->next = c->next;
            free(c); 
            break;
        }
        p = c; c = c->next;
    }
    pthread_mutex_unlock(&mutexUsers);
}

// Analyse les messages entrants selon le protocole BEUIP (UDP port 9998)
void traiter_requete(char code, char *data, char *ip_client) {
    if (code == '1') { // Nouveau venu sur le réseau
        ajouteElt(data, ip_client);
        char msg[512]; 
        snprintf(msg, sizeof(msg), "2BEUIP%s", monPseudo_global); // Réponse avec code '2' (Accusé)
        struct sockaddr_in cl; 
        memset(&cl, 0, sizeof(cl)); 
        cl.sin_family=AF_INET; cl.sin_port=htons(PORT); cl.sin_addr.s_addr=inet_addr(ip_client);
        sendto(sid_global, msg, strlen(msg)+1, 0, (struct sockaddr *)&cl, sizeof(cl));
    } 
    else if (code == '2') ajouteElt(data, ip_client); // Réception d'un accusé d'identification
    else if (code == '0') supprimeElt(ip_client); // Départ d'un utilisateur
    else if (code == '9') printf("%s\n", data); // Affichage brut pour la messagerie (Consigne 10)
}

// Boucle infinie du thread serveur UDP
void *serveur_udp(void *p) {
    (void)p; // Suppression du warning unused-parameter pour compiler avec -Werror
    struct sockaddr_in cl; socklen_t ls; char buf[512];
    while (serveur_actif) {
        ls = sizeof(cl); 
        int n = recvfrom(sid_global, buf, 511, 0, (struct sockaddr *)&cl, &ls);
        // On vérifie l'en-tête "BEUIP" pour valider le message
        if (n > 6 && strncmp(buf + 1, "BEUIP", 5) == 0) {
            buf[n] = '\0';
            // On ignore nos propres broadcasts, sauf les messages de texte (code '9')
            if (strcmp(buf + 6, monPseudo_global) != 0 || buf[0] == '9') 
                traiter_requete(buf[0], buf + 6, inet_ntoa(cl.sin_addr));
        }
    }
    return NULL;
}

// Initialise la socket UDP et envoie le broadcast de présence
pthread_t init_serveur_udp(char *pseudo) {
    struct sockaddr_in Conf, Bcast;
    strncpy(monPseudo_global, pseudo, LPSEUDO); 
    monPseudo_global[LPSEUDO] = '\0';
    serveur_actif = 1;
    
    sid_global = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int on = 1; 
    setsockopt(sid_global, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)); // Autorisation du broadcast
    
    memset(&Conf, 0, sizeof(Conf)); 
    Conf.sin_family = AF_INET; Conf.sin_port = htons(PORT); Conf.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sid_global, (struct sockaddr *)&Conf, sizeof(Conf));
    
    // Envoi sur l'IP de broadcast définie dans beuip_shared.h
    memset(&Bcast, 0, sizeof(Bcast)); 
    Bcast.sin_family = AF_INET; Bcast.sin_port = htons(PORT); Bcast.sin_addr.s_addr = inet_addr(BCAST_IP);
    char buf[512]; 
    snprintf(buf, sizeof(buf), "1BEUIP%s", monPseudo_global);
    sendto(sid_global, buf, strlen(buf)+1, 0, (struct sockaddr *)&Bcast, sizeof(Bcast));
    
    pthread_t tid; 
    pthread_create(&tid, NULL, serveur_udp, NULL); // Création du thread
    return tid;
}

// Redirige la sortie du processus fils vers la socket TCP pour le transfert
void envoiContenu(int fd, char *rep) {
    char buf[256];
    if (read(fd, buf, 1) <= 0) { close(fd); return; }
    
    if (buf[0] == 'L' && fork() == 0) { // Commande ls -l
        dup2(fd, STDOUT_FILENO); dup2(fd, STDERR_FILENO); close(fd); // Redirection stdout/stderr
        execlp("ls", "ls", "-l", rep, NULL); 
        exit(1);
    } 
    else if (buf[0] == 'F') { // Envoi de fichier via cat
        int i = 0; 
        while (read(fd, &buf[i], 1) > 0 && buf[i] != '\n' && i < 255) i++;
        buf[i] = '\0';
        
        if (fork() == 0) { 
            dup2(fd, STDOUT_FILENO); dup2(fd, STDERR_FILENO); close(fd);
            char path[512]; 
            snprintf(path, sizeof(path), "%s/%s", rep, buf);
            execlp("cat", "cat", path, NULL); 
            exit(1);
        }
    }
    close(fd); // Le père ferme la socket de ce client, les fils gèrent les flux
}

// Thread serveur TCP dédié à l'échange de fichiers
void *serveur_tcp(void *rep) {
    struct sockaddr_in cl; socklen_t ls = sizeof(cl);
    while (serveur_actif) {
        int fd = accept(sid_tcp_global, (struct sockaddr *)&cl, &ls);
        if (fd >= 0) envoiContenu(fd, (char*)rep);
    }
    return NULL;
}

// Initialise l'écoute TCP sur le port 9998
pthread_t init_serveur_tcp(char *rep) {
    struct sockaddr_in Conf;
    sid_tcp_global = socket(AF_INET, SOCK_STREAM, 0);
    
    int on = 1; setsockopt(sid_tcp_global, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
    memset(&Conf, 0, sizeof(Conf)); 
    Conf.sin_family = AF_INET; Conf.sin_port = htons(PORT); Conf.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sid_tcp_global, (struct sockaddr *)&Conf, sizeof(Conf)); 
    listen(sid_tcp_global, 5);
    
    pthread_t tid; 
    pthread_create(&tid, NULL, serveur_tcp, rep); // Deuxième thread
    return tid;
}