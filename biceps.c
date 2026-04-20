#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "gescom.h"
#include "beuip_shared.h"

#define NBMAXC 15

typedef struct { char *nom; int (*fonction)(int, char **); } CommandeInterne;
static CommandeInterne TabCom[NBMAXC]; static int NbComInt = 0; 
pthread_t tid_udp, tid_tcp; 

int CommandeBeuip(int N, char *P[]); 

int execComInt(int N, char **P) {
    if (N == 0 || P[0] == NULL) return 0;
    for (int i = 0; i < NbComInt; i++) { 
        if (strcmp(P[0], TabCom[i].nom) == 0) { TabCom[i].fonction(N, P); return 1; } 
    }
    return 0; 
}

int Sortie(int N, char *P[]) { 
    (void)N; (void)P; 
    
    if (serveur_actif) { 
        char *args[] = {"beuip", "stop"}; 
        CommandeBeuip(2, args); 
    }
    
    liberer_liste(); 
    liberer_gescom(); 
    clear_history(); 
    
    exit(0); return 0; 
}

int ChangeDir(int N, char *P[]) { 
    if (N > 1) chdir(P[1]); else chdir(getenv("HOME")); 
    return 0; 
}

int PrintWD(int N, char *P[]) { 
    (void)N; (void)P; 
    char cwd[1024]; 
    if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd); 
    return 0; 
}

// --- DECOUPAGE DES FONCTIONS POUR RESPECTER LA REGLE DES < 20 LIGNES ---

void gerer_serveurs(int N, char *P[]) {
    if (strcmp(P[1], "start") == 0 && N >= 3 && !serveur_actif) {
        tid_udp = init_serveur_udp(P[2]); 
        tid_tcp = init_serveur_tcp("reppub"); 
    } 
    else if (strcmp(P[1], "stop") == 0 && serveur_actif) {
        commande('0', NULL, NULL); serveur_actif = 0; 
        struct sockaddr_in self; memset(&self, 0, sizeof(self));
        self.sin_family = AF_INET; self.sin_port = htons(PORT); self.sin_addr.s_addr = inet_addr("127.0.0.1");
        sendto(sid_global, "W", 1, 0, (struct sockaddr *)&self, sizeof(self));
        int sock = socket(AF_INET, SOCK_STREAM, 0); connect(sock, (struct sockaddr *)&self, sizeof(self)); close(sock);
        pthread_join(tid_udp, NULL); pthread_join(tid_tcp, NULL);
        if (sid_global != -1) close(sid_global);
        if (sid_tcp_global != -1) close(sid_tcp_global);
    }
}

void gerer_messagerie(int N, char *P[]) {
    char msg[512] = ""; 
    size_t len = 0;
    for(int i = 3; i < N; i++) { 
        // Sécurité stricte : empêche le Buffer Overflow (Segfault)
        if (len + strlen(P[i]) + 2 >= sizeof(msg)) break; 
        strcat(msg, P[i]); 
        if(i < N-1) strcat(msg, " "); 
        len += strlen(P[i]) + 1;
    }
    
    if (strcmp(P[2], "all") == 0) commande('5', msg, NULL);
    else commande('4', msg, P[2]);
}

// Fonction principale d'aiguillage allégée (< 15 lignes)
int CommandeBeuip(int N, char *P[]) {
    if (N < 2) return 0;

    if (strcmp(P[1], "start") == 0 || strcmp(P[1], "stop") == 0) gerer_serveurs(N, P);
    else if (strcmp(P[1], "message") == 0 && N >= 4) gerer_messagerie(N, P);
    else if (strcmp(P[1], "list") == 0) commande('3', NULL, NULL);
    else if (strcmp(P[1], "ls") == 0 && N >= 3) demandeListe(P[2]);
    else if (strcmp(P[1], "get") == 0 && N >= 4) demandeFichier(P[2], P[3]);
    
    return 0;
}

// -----------------------------------------------------------------------

void ajouteCom(char *nom, int (*f)(int, char **)) { TabCom[NbComInt].nom = nom; TabCom[NbComInt].fonction = f; NbComInt++; }

void majComInt(void) {
    ajouteCom("exit", Sortie); 
    ajouteCom("cd", ChangeDir); 
    ajouteCom("pwd", PrintWD);
    ajouteCom("beuip", CommandeBeuip); 
}

int main() {
    signal(SIGINT, SIG_IGN); 
    signal(SIGCHLD, SIG_IGN); 
    
    majComInt(); 
    read_history(".biceps_history");
    
    char prompt[512] = "biceps$ ";
    char *cmd;
    
    while ((cmd = readline(prompt)) != NULL) {
        if (strlen(cmd) > 0) {
            add_history(cmd); 
            char *reste = cmd, *seq;
            while ((seq = strsep(&reste, ";")) != NULL) {
                if (strlen(seq) > 0 && analyseCom(seq) > 0) { 
                    if (execComInt(NMots, Mots) == 0) execComExt(Mots); 
                }
            }
        }
        free(cmd); 
    }
    Sortie(0, NULL); 
    return 0;
}