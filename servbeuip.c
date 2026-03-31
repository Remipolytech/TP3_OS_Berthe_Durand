#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 9998
#define LBUF 512
#define MAX_USERS 255
#define BCAST_IP "192.168.88.255" // Adresse broadcast du réseau TPOSUSER

typedef struct {
    struct in_addr ip;
    char pseudo[50];
} User;

User tableUsers[MAX_USERS];
int nbUsers = 0;

// --- Variables globales pour le signal d'arrêt ---
int sid_global;
struct sockaddr_in BcastSock_global;
char *monPseudo_global;

void handle_stop_signal(int sig) {
    printf("\n[!] Signal reçu. Arrêt du serveur BEUIP...\n");
    char buf[LBUF];
    buf[0] = '0';
    memcpy(buf + 1, "BEUIP", 5);
    strcpy(buf + 6, monPseudo_global);
    
    // Envoi du message '0' pour prévenir tout le monde
    sendto(sid_global, buf, 6 + strlen(monPseudo_global) + 1, 0, (struct sockaddr *)&BcastSock_global, sizeof(BcastSock_global));
    exit(0);
}
// -------------------------------------------------

void ajouterUser(struct in_addr ip, char *pseudo) {
    for (int i = 0; i < nbUsers; i++) {
        if (tableUsers[i].ip.s_addr == ip.s_addr) {
            // Mise à jour du pseudo au cas où il aurait changé
            strncpy(tableUsers[i].pseudo, pseudo, 49);
            return; 
        }
    }
    if (nbUsers < MAX_USERS) {
        tableUsers[nbUsers].ip = ip;
        strncpy(tableUsers[nbUsers].pseudo, pseudo, 49);
        tableUsers[nbUsers].pseudo[49] = '\0'; // Sécurité
        nbUsers++;
        printf("[+] %s a rejoint le réseau (%s)\n", pseudo, inet_ntoa(ip));
    }
}

void supprimerUser(char *pseudo) {
    for (int i = 0; i < nbUsers; i++) {
        if (strcmp(tableUsers[i].pseudo, pseudo) == 0) {
            tableUsers[i] = tableUsers[nbUsers - 1];
            nbUsers--;
            printf("[-] %s a quitté le réseau.\n", pseudo);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pseudo>\n", argv[0]);
        return 1;
    }

    char *monPseudo = argv[1];
    monPseudo_global = monPseudo;
    signal(SIGUSR1, handle_stop_signal); 

    int sid;
    struct sockaddr_in SockConf, BcastSock, ClientSock;
    socklen_t ls;
    char buf[LBUF];

    if ((sid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket"); return 2;
    }

    int broadcastEnable = 1;
    setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    memset(&SockConf, 0, sizeof(SockConf));
    SockConf.sin_family = AF_INET;
    SockConf.sin_port = htons(PORT);
    SockConf.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sid, (struct sockaddr *)&SockConf, sizeof(SockConf)) < 0) {
        perror("bind"); return 4;
    }
    printf("Serveur BEUIP (%s) en écoute sur le port %d...\n", monPseudo, PORT);

    memset(&BcastSock, 0, sizeof(BcastSock));
    BcastSock.sin_family = AF_INET;
    BcastSock.sin_port = htons(PORT);
    BcastSock.sin_addr.s_addr = inet_addr(BCAST_IP);

    sid_global = sid;
    BcastSock_global = BcastSock;

    buf[0] = '1';
    memcpy(buf + 1, "BEUIP", 5);
    strcpy(buf + 6, monPseudo);
    sendto(sid, buf, 6 + strlen(monPseudo) + 1, 0, (struct sockaddr *)&BcastSock, sizeof(BcastSock));

    for (;;) {
        ls = sizeof(ClientSock);
        // LBUF - 1 pour garder la place du \0 final
        int n = recvfrom(sid, buf, LBUF - 1, 0, (struct sockaddr *)&ClientSock, &ls);
        
        if (n > 0) {
            buf[n] = '\0'; // on force la fin de chaîne proprement
            
            if (n > 6 && strncmp(buf + 1, "BEUIP", 5) == 0) {
                char code = buf[0];
                char *pseudoExpediteur = buf + 6;
                int isLocal = (strcmp(inet_ntoa(ClientSock.sin_addr), "127.0.0.1") == 0);

                if ((code == '1' || code == '2' || code == '0') && strcmp(pseudoExpediteur, monPseudo) == 0) continue;

                if (code == '1') { 
                    ajouterUser(ClientSock.sin_addr, pseudoExpediteur);
                    char msgAR[LBUF];
                    msgAR[0] = '2';
                    memcpy(msgAR + 1, "BEUIP", 5);
                    strcpy(msgAR + 6, monPseudo);
                    sendto(sid, msgAR, 6 + strlen(monPseudo) + 1, 0, (struct sockaddr *)&ClientSock, ls);
                } 
                else if (code == '2') { 
                    ajouterUser(ClientSock.sin_addr, pseudoExpediteur);
                }
                else if (code == '0') {
                    supprimerUser(pseudoExpediteur);
                }
                else if (code == '9') { 
                    char *senderPseudo = "Inconnu";
                    for (int i = 0; i < nbUsers; i++) {
                        if (tableUsers[i].ip.s_addr == ClientSock.sin_addr.s_addr) {
                            senderPseudo = tableUsers[i].pseudo;
                            break;
                        }
                    }
                    printf("\n\a[Message de %s] : %s\n", senderPseudo, pseudoExpediteur);
                }
                else if (isLocal) {
                    if (code == '3') {
                        printf("\n=== Liste des utilisateurs (%d) ===\n", nbUsers);
                        if (nbUsers == 0) printf("  Personne d'autre.\n");
                        for (int i = 0; i < nbUsers; i++) printf("  - %s (%s)\n", tableUsers[i].pseudo, inet_ntoa(tableUsers[i].ip));
                        printf("=================================\n");
                    }
                    else if (code == '4') { 
                        char *destPseudo = pseudoExpediteur;
                        char *message = destPseudo + strlen(destPseudo) + 1;
                        int found = 0;
                        
                        for (int i = 0; i < nbUsers; i++) {
                            if (strcmp(tableUsers[i].pseudo, destPseudo) == 0) {
                                char msgEnvoi[LBUF];
                                msgEnvoi[0] = '9';
                                memcpy(msgEnvoi + 1, "BEUIP", 5);
                                strcpy(msgEnvoi + 6, message);
                                
                                struct sockaddr_in destSock;
                                memset(&destSock, 0, sizeof(destSock));
                                destSock.sin_family = AF_INET;
                                destSock.sin_port = htons(PORT);
                                destSock.sin_addr = tableUsers[i].ip;
                                
                                sendto(sid, msgEnvoi, 6 + strlen(message) + 1, 0, (struct sockaddr *)&destSock, sizeof(destSock));
                                printf("[->] Message privé envoyé à %s.\n", destPseudo);
                                found = 1; break;
                            }
                        }
                        if (!found) printf("[!] Erreur : %s introuvable.\n", destPseudo);
                    }
                    else if (code == '5') { 
                        char *message = pseudoExpediteur;
                        char msgEnvoi[LBUF];
                        msgEnvoi[0] = '9'; 
                        memcpy(msgEnvoi + 1, "BEUIP", 5);
                        strcpy(msgEnvoi + 6, message);
                        
                        for (int i = 0; i < nbUsers; i++) {
                            struct sockaddr_in destSock;
                            memset(&destSock, 0, sizeof(destSock));
                            destSock.sin_family = AF_INET;
                            destSock.sin_port = htons(PORT);
                            destSock.sin_addr = tableUsers[i].ip;
                            sendto(sid, msgEnvoi, 6 + strlen(message) + 1, 0, (struct sockaddr *)&destSock, sizeof(destSock));
                        }
                        printf("[->] Message envoyé à %d personne(s).\n", nbUsers);
                    }
                }
            }
        }
    }
    return 0;
}