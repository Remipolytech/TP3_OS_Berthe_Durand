#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "gescom.h"
#include "creme.h" // INCLUSION DE NOTRE NOUVELLE LIBRAIRIE !

#define NBMAXC 15 // Augmenté pour nos nouvelles commandes

typedef struct {
    char *nom;
    int (*fonction)(int, char **);
} CommandeInterne;

static CommandeInterne TabCom[NBMAXC]; 
static int NbComInt = 0; 

// --- Variable pour stocker le PID du serveur BEUIP -
pid_t beuip_server_pid = -1;

// Cherche et exécute une commande interne
int execComInt(int N, char **P) {
    if (N == 0 || P[0] == NULL) return 0;
    for (int i = 0; i < NbComInt; i++) {
        if (strcmp(P[0], TabCom[i].nom) == 0) {
            TabCom[i].fonction(N, P); 
            return 1; 
        }
    }
    return 0; 
}

// -- COMMANDES INTERNES CLASSIQUES --

int Sortie(int N, char *P[]) { 
    if (beuip_server_pid != -1) {
        printf("Arrêt du serveur BEUIP avant de quitter...\n");
        kill(beuip_server_pid, SIGUSR1);
    }
    printf("Bye ! Merci d'avoir utilisé biceps.\n");
    write_history(".biceps_history"); 
    exit(0); 
    return 0; 
}

int ChangeDir(int N, char *P[]) {
    if (N > 1) {
        if (chdir(P[1]) != 0) perror("Erreur cd");
    } else {
        chdir(getenv("HOME"));
    }
    return 0;
}

int PrintWD(int N, char *P[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("Erreur pwd");
    }
    return 0;
}

int Version(int N, char *P[]) {
    printf("BICEPS v2 - Avec intégration BEUIP !\n");
    return 0;
}

// -- NOUVELLES COMMANDES BEUIP --

int CommandeBeuip(int N, char *P[]) {
    if (N < 2) {
        printf("Usage: beuip start <pseudo> | beuip stop\n");
        return 0;
    }

    if (strcmp(P[1], "start") == 0) {
        if (N < 3) {
            printf("Usage: beuip start <pseudo>\n");
            return 0;
        }
        if (beuip_server_pid != -1) {
            printf("Le serveur BEUIP tourne déjà (PID: %d).\n", beuip_server_pid);
            return 0;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            // Processus fils : on lance le serveur
            char *args[] = {"./servbeuip", P[2], NULL};
            execvp(args[0], args);
            perror("Erreur execvp servbeuip");
            exit(1);
        } else {
            // Processus père (biceps) : on mémorise le PID
            beuip_server_pid = pid;
            printf("Serveur BEUIP lancé en tâche de fond (PID: %d).\n", pid);
            sleep(1); // Laisse une seconde au serveur pour s'afficher proprement
        }
    } 
    else if (strcmp(P[1], "stop") == 0) {
        if (beuip_server_pid == -1) {
            printf("Aucun serveur BEUIP n'est en cours d'exécution.\n");
            return 0;
        }
        printf("Envoi du signal d'arrêt au serveur BEUIP...\n");
        kill(beuip_server_pid, SIGUSR1);
        waitpid(beuip_server_pid, NULL, 0); // On attend qu'il se termine bien
        beuip_server_pid = -1;
        printf("Serveur BEUIP arrêté.\n");
    } else {
        printf("Commande beuip inconnue.\n");
    }
    return 0;
}

int CommandeMess(int N, char *P[]) {
    if (N < 2) {
        printf("Usage: mess liste | mess mp <pseudo> <msg> | mess all <msg>\n");
        return 0;
    }

    if (strcmp(P[1], "liste") == 0) {
        creme_send_commande('3', NULL, NULL);
    } 
    else if (strcmp(P[1], "mp") == 0 && N >= 4) {
        char message[512] = "";
        for(int i = 3; i < N; i++) { strcat(message, P[i]); strcat(message, " "); }
        message[strlen(message)-1] = '\0';
        creme_send_commande('4', P[2], message);
    } 
    else if (strcmp(P[1], "all") == 0 && N >= 3) {
        char message[512] = "";
        for(int i = 2; i < N; i++) { strcat(message, P[i]); strcat(message, " "); }
        message[strlen(message)-1] = '\0';
        creme_send_commande('5', NULL, message);
    } 
    else {
        printf("Commande mess invalide.\n");
    }
    return 0;
}

// ---------------------------------

void ajouteCom(char *nom, int (*f)(int, char **)) {
    if (NbComInt >= NBMAXC) {
        fprintf(stderr, "Erreur fatale : Tableau plein !\n");
        exit(1); 
    }
    TabCom[NbComInt].nom = nom;
    TabCom[NbComInt].fonction = f;
    NbComInt++;
}

void majComInt(void) {
    ajouteCom("exit", Sortie); 
    ajouteCom("cd", ChangeDir);
    ajouteCom("pwd", PrintWD);
    ajouteCom("vers", Version);
    ajouteCom("beuip", CommandeBeuip); 
    ajouteCom("mess", CommandeMess);   
}

int main() {
    signal(SIGINT, SIG_IGN); 

    majComInt();
    read_history(".biceps_history");

    char *user = getenv("USER");
    if (user == NULL) user = "inconnu";

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) strcpy(hostname, "machine");

    char prompt_char = (geteuid() == 0) ? '#' : '$';

    size_t prompt_len = strlen(user) + strlen(hostname) + 4;
    char *prompt = (char *)malloc(prompt_len);
    if (prompt == NULL) return 1;
    snprintf(prompt, prompt_len, "%s@%s%c ", user, hostname, prompt_char);

    char *commande;

    while (1) {
        commande = readline(prompt);

        if (commande == NULL) {
            Sortie(0, NULL);
            break; 
        }

        if (strlen(commande) > 0) {
            add_history(commande); 

            char *reste_seq = commande;
            char *cmd_seq;

            while ((cmd_seq = strsep(&reste_seq, ";")) != NULL) {
                if (strlen(cmd_seq) > 0) {
                    int n = analyseCom(cmd_seq);
                    if (n > 0) {
                        if (execComInt(n, Mots) == 0) {
                            execComExt(Mots);
                        }
                    }
                }
            }
        }
        free(commande); 
    }
    
    free(prompt);
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }

    return 0;
}