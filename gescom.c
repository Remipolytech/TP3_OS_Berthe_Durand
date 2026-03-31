#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "gescom.h"

// Définition de nos variables globale
char **Mots = NULL; 
int NMots = 0;      

int analyseCom(char *b) {
    char *token;
    char *delim = " \t\n"; 
    
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }
    NMots = 0;
    
    int max_args = 50;
    Mots = (char **)malloc(max_args * sizeof(char *));
    if (Mots == NULL) return 0;

    char *reste = b; 
    while ((token = strsep(&reste, delim)) != NULL) {
        if (strlen(token) > 0) {
            Mots[NMots++] = strdup(token); 
            if (NMots >= max_args - 1) break; 
        }
    }
    Mots[NMots] = NULL; 
    return NMots; 
}

int execComExt(char **P) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Erreur fork");
        return 0;
    } else if (pid == 0) {
        if (execvp(P[0], P) == -1) {
            perror("Erreur d'exécution");
            exit(1);
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
    return 1;
}