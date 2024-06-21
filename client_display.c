#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
    unsigned short int type;
    unsigned short int orig_uid;
    unsigned short int dest_uid;
    unsigned short int text_len;
    unsigned char text[141];
} msg_t;

// client_display.c

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ID> <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    msg_t message;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));

    if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    message.type = 0; // Oi message
    message.orig_uid = atoi(argv[1]);
    send(sock, &message, sizeof(message), 0);

    while ((valread = recv(sock, &message, sizeof(message), 0)) > 0) {
        printf("Mensagem de %d: %s\n", message.orig_uid, message.text); // Adicione esta linha para debug
    }

    return 0;
}