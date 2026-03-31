#ifndef GESCOM_H
#define GESCOM_H

//On prévient les autres fichiers que ces variables globales existent ailleurs
extern char **Mots;
extern int NMots;

// Déclarationn de nos fonctions clés
int analyseCom(char *b);
int execComExt(char **P);

#endif