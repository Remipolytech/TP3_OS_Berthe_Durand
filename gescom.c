#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "gescom.h"

char **Mots = NULL; 
int NMots = 0;      

// Vide proprement la mémoire allouée par le parseur de commandes
void liberer_gescom(void) {
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
        Mots = NULL;
        NMots = 0;
    }
}

// Découpe la ligne tapée par l'utilisateur en un tableau de mots
int analyseCom(char *b) {
    char *token, *delim = " \t\n"; 
    
    // On nettoie la commande précédente avant d'en analyser une nouvelle
    liberer_gescom();
    
    int max_args = 50;
    // calloc est plus sûr ici car il initialise la mémoire à zéro
    Mots = (char **)calloc(max_args, sizeof(char *));
    if (Mots == NULL) return 0;

    char *reste = b; 
    while ((token = strsep(&reste, delim)) != NULL) {
        if (strlen(token) > 0) {
            Mots[NMots++] = strdup(token); // Allocation dynamique de chaque mot
            if (NMots >= max_args - 1) break; 
        }
    }
    Mots[NMots] = NULL; // Le dernier élément doit être NULL pour execvp
    return NMots; 
}

// Exécute une commande système classique (ls, cat, etc.)
int execComExt(char **P) {
    pid_t pid = fork();
    if (pid == 0) {
        // Le processus fils remplace son code par la commande système
        if (execvp(P[0], P) == -1) exit(1);
    } else if (pid > 0) {
        // Le père attend que la commande système soit terminée
        waitpid(pid, NULL, 0);
    }
    return 1;
}