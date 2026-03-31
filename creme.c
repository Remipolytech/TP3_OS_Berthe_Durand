#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "creme.h"

#define PORT 9998
#define SERVER_IP "127.0.0.1"

int creme_send_commande(char code, char *destinataire, char *message) {
    int sid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sid < 0) return -1;

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    serv.sin_addr.s_addr = inet_addr(SERVER_IP);

    char buf[512];
    buf[0] = code;
    memcpy(buf + 1, "BEUIP", 5);

    int len = 6;
    if (code == '3') {
        buf[len] = '\0';
        len += 1;
    } 
    else if (code == '4' && destinataire && message) {
        strcpy(buf + len, destinataire);
        len += strlen(destinataire) + 1;
        strcpy(buf + len, message);
        len += strlen(message) + 1;
    } 
    else if (code == '5' && message) {
        strcpy(buf + len, message);
        len += strlen(message) + 1;
    } else {
        return -1;
    }

    sendto(sid, buf, len, 0, (struct sockaddr *)&serv, sizeof(serv));
    return 0;
}