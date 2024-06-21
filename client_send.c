#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>

#define MAX_CLIENTS 20
#define BUFFER_SIZE 1024

typedef struct {
    unsigned short int type;
    unsigned short int orig_uid;
    unsigned short int dest_uid;
    unsigned short int text_len;
    unsigned char text[141];
} msg_t;

int clients[MAX_CLIENTS];
fd_set read_fds, temp_fds;
int max_sd, server_fd;
struct timeval timeout;
volatile sig_atomic_t timer_expired = 0;

void handle_signal(int signal) {
    timer_expired = 1;
}

void setup_server(int port) {
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Bind successful on port %d\n", port);

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d\n", port);

    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    max_sd = server_fd;
}

void accept_new_connection() {
    int new_socket, addrlen, i;
    struct sockaddr_in address;
    addrlen = sizeof(address);

    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("New connection accepted\n");

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == 0) {
            clients[i] = new_socket;
            FD_SET(new_socket, &read_fds);
            if (new_socket > max_sd) {
                max_sd = new_socket;
            }
            break;
        }
    }
}

void handle_client_message(int client_fd) {
    int valread;
    msg_t message;
    valread = recv(client_fd, &message, sizeof(message), 0);
    if (valread == 0) {
        close(client_fd);
        FD_CLR(client_fd, &read_fds);
        printf("Connection closed\n");
    } else {
        switch (message.type) {
            case 0: // OI message
                printf("Received OI from client %d\n", message.orig_uid);
                break;
            case 1: // TCHAU message
                printf("Received TCHAU from client %d\n", message.orig_uid);
                close(client_fd);
                FD_CLR(client_fd, &read_fds);
                break;
            case 2: // MSG type
                printf("Message received from client %d: %s\n", message.orig_uid, message.text);
                if (message.dest_uid == 0) {
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] != 0 && clients[i] != client_fd) {
                            send(clients[i], &message, sizeof(message), 0);
                            printf("Message forwarded to client %d\n", clients[i]);
                        }
                    }
                } else {
                    // Forward to specific client
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] != 0 && clients[i] != client_fd) {
                            if (clients[i] == message.dest_uid) {
                                send(clients[i], &message, sizeof(message), 0);
                                printf("Message forwarded to specific client %d\n", clients[i]);
                                break;
                            }
                        }
                    }
                }
                break;
            default:
                printf("Unknown message type received from client %d\n", message.orig_uid);
                break;
        }
    }
}

void send_periodic_message() {
    int i;
    msg_t message;
    message.type = 2; // MSG type
    message.orig_uid = 0; // Server ID
    snprintf((char *)message.text, sizeof(message.text), "Server status: ...");

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0) {
            send(clients[i], &message, sizeof(message), 0);
        }
    }
}

void start_server() {
    int activity, i;
    while (1) {
        temp_fds = read_fds;
        activity = select(max_sd + 1, &temp_fds, NULL, NULL, &timeout);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &temp_fds)) {
            accept_new_connection();
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(clients[i], &temp_fds)) {
                handle_client_message(clients[i]);
            }
        }

        if (timer_expired) {
            send_periodic_message();
            timer_expired = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, handle_signal);
    setup_server(atoi(argv[1]));
    alarm(60); // Set timer for periodic messages

    start_server();

    return 0;
}