#ifndef CREME_H
#define CREME_H

// Fonction pour envoyer des ordres locaux au serveur BEUIP
int creme_send_commande(char code, char *destinataire, char *message);

#endif