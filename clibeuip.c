#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9998
#define SERVER_IP "127.0.0.1"
#define LBUF 512

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s liste\n", argv[0]);
        printf("  %s mp <pseudo> <message>\n", argv[0]);
        printf("  %s all <message>\n", argv[0]);
        return 1;
    }

    int sid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    serv.sin_addr.s_addr = inet_addr(SERVER_IP);

    char buf[LBUF];

    if (strcmp(argv[1], "liste") == 0) {
        buf[0] = '3';
        memcpy(buf + 1, "BEUIP", 5);
        buf[6] = '\0'; 
        sendto(sid, buf, 7, 0, (struct sockaddr *)&serv, sizeof(serv));
    } 
    else if (strcmp(argv[1], "mp") == 0 && argc >= 4) {
        char *pseudo = argv[2];
        char message[256] = "";
        for(int i = 3; i < argc; i++) { strcat(message, argv[i]); strcat(message, " "); }
        message[strlen(message)-1] = '\0'; // retirer l'espace final

        buf[0] = '4';
        memcpy(buf + 1, "BEUIP", 5);
        strcpy(buf + 6, pseudo);
        strcpy(buf + 6 + strlen(pseudo) + 1, message); // On copie le message juste après le \0 du pseudo
        
        sendto(sid, buf, 6 + strlen(pseudo) + 1 + strlen(message) + 1, 0, (struct sockaddr *)&serv, sizeof(serv));
    }
    else if (strcmp(argv[1], "all") == 0 && argc >= 3) {
        char message[256] = "";
        for(int i = 2; i < argc; i++) { strcat(message, argv[i]); strcat(message, " "); }
        message[strlen(message)-1] = '\0';

        buf[0] = '5';
        memcpy(buf + 1, "BEUIP", 5);
        strcpy(buf + 6, message);
        sendto(sid, buf, 6 + strlen(message) + 1, 0, (struct sockaddr *)&serv, sizeof(serv));
    }
    else {
        printf("Commande inconnue ou incomplète.\n");
    }

    return 0;
}