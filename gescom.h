#ifndef GESCOM_H
#define GESCOM_H

// Variables exposées pour que biceps puisse lire la commande analysée
extern char **Mots;
extern int NMots;

int analyseCom(char *b);
int execComExt(char **P);
void liberer_gescom(void); // Nettoie le tableau de mots pour Valgrind

#endif