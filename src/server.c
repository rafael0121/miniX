/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

/* System library */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/ip.h>
#include <string.h>
#include <signal.h>

/* Program library */
#include <handlesocket.h>
#include <server.h>
#include <msg.h>
#include <time.h>

/*============================================================================*
 * Global Variables                                                           *
 *============================================================================*/

int count_clients = 0;

/*============================================================================*
 * Structs                                                                    *
 *============================================================================*/

struct {
    int id;
    int fd;
} client_a[20];

/*============================================================================*
 * Sockets Functions                                                          *
 *============================================================================*/

/**
 * @brief Accepts a connection from the socket and
 * creates a new socket connected to the client.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative errno.
 */
int accept_connection(int sockfd)
{
    struct sockaddr addr;
    socklen_t length = 0;

    int ret = accept(sockfd, &addr, &length);

    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Failed to accept a connection. errno: %i \n", errno);
    }

    return ret;
}

/**
 * @brief Bind a address to the socket.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative errno.
 */
int setaddr_socket(int sockfd, int port)
{

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the addres to the socket.
    int ret = bind(sockfd, (struct sockaddr *) (&addr), sizeof(struct sockaddr));

    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Failed to bind the address. errno: %i \n", errno);
    }

    return ret;
}

/**
 * @brief Set the socket to listen and accept connections
 * the length of the queue for pending connections is defined
 * by MAX_QUEUE.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative errno.
 */
int listen_socket(int sockfd)
{

    // Set socket to listen.
    int ret = listen(sockfd, MAX_QUEUE);

    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Failed to set socket to listen. errno: %i \n", errno);
    }

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr *) &addr, &len) < 0){
        fprintf(stderr, "\n ### ERROR: Failed to get socket port. errno: %i \n", errno);
    } else {
        fprintf(stderr, "\n Listening on %i\n", addr.sin_port);
    }

    return ret;
}

/**
 * @brief Get value from cli arguments.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative error.
 */
int get_port(int argc, char *argv[], int *port)
{
    // Check numbers of args.
    if (argc != 2) {
        fprintf(stderr, "\n ### ERROR: Missing Arguments.\n");
        HELPSERVER(argv[0]);
        return -1;
    }

    // Get port and check if is valid.
    *port = atoi(argv[1]);
    if (*port < 1024 || *port > 65536) {
        fprintf(stderr, "\n ### ERROR: PORT is out of limit.\n");
        HELPSERVER(argv[0]);
        return -1;
    }
    return 0;
}

/*============================================================================*
 * Service Menssage Functions                                                   *
 *============================================================================*/

/**
 * @brief Service OI messages.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative error.
 */
int service_oi(struct msg_t msg, int sockfd) {

    int id = msg.orig_uid;

    // Check if exist a client using the same id.
    for (int i = 0; i < 20; i++) {
        if (client_a[i].id == id || client_a[i].id == (id + 1000)) {
            fprintf(stderr, "\n ### ERROR: [OI] ID already in use.\n");
            // Resend the same message without swap dest and orig.
            send_message(sockfd, msg);
            return -1;
        }
    }

    // Swap orig and dest.
    msg.orig_uid = msg.dest_uid;
    msg.dest_uid = id;

    // Reader client.
    if (id > 0 && id < 1000) {
        // Add new reader
        for (int i = 0; i < 10; i++) {
            if (client_a[i].id == -1) {
                client_a[i].id = id;
                client_a[i].fd = sockfd;

                // Send back OI message.
                if (send_message(sockfd, msg) == -1){
                    return -1;
                } else {
                    count_clients+=1;
                    return 0;
                }
            }
        }
        fprintf(stderr, "\n ### ERROR: [OI] Server full.\n");
        return -1;
    }

    // Sender client.
    if (id > 1000 && id < 2000) {
        // Add new sender
        for (int i = 10; i < 20; i++) {
            if (client_a[i].id == -1) {
                client_a[i].id = id;
                client_a[i].fd = sockfd;
                // Send back OI message.
                if (send_message(sockfd, msg) == -1){
                    return -1;
                } else {
                    count_clients+=1;
                    return 0;
                }
            }
        }
        fprintf(stderr, "\n ### ERROR: [OI] Server full.\n");
        return -1;
    }

    fprintf(stderr, "\n ### ERROR: [OI] Incorrect ID.\n");

    return -1;
}

/**
 * @brief Service TCHAU messages.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative error.
 */
int service_tchau(int sockfd) {

    // Remove client.
    for (int i = 0; i < 20; i++) {
        if (client_a[i].fd == sockfd) {
            client_a[i].id = -1;
            client_a[i].fd = -1;
            count_clients-=1;
        }
    }

    // Create tchau response message.
    struct msg_t tchau_msg;
    tchau_msg.type = TCHAU;
    tchau_msg.orig_uid = 0;
    tchau_msg.dest_uid = -1;
    tchau_msg.text_len = 0;

    send_message(sockfd, tchau_msg);

    return 0;
}

/**
 * @brief Service MSG messages.
 *
 * @return Upon succesfull zero is returned. otherwise
 * a negative error.
 */
int service_msg(struct msg_t msg) {

    int orig = msg.orig_uid;
    int dest = msg.dest_uid;

    // Check if dest is a sender.
    if ((dest > 1000) || (dest < 0)) {
        fprintf(stderr, "\n ### ERROR: [MSG] Client dest not is a reader.\n");
        return -1;
    }
    // Check if client is registered.
    int aux = -1;
    for (int i = 10; i < 20; i++) {
        if (client_a[i].id == orig) {
            aux = 0;
            break;
        }
    }
    if (aux == -1) {
        fprintf(stderr, "\n ### ERROR: [MSG] Sender not registered.\n");
        return -1;
    }

    // Send to only a dest.
    if (dest > 0 && dest < 1000) {
        /* Get dest socket*/
        int dest_fd;
        aux = -1;
        for (int i = 0; i < 10; i++) {
            if (client_a[i].id == msg.dest_uid) {
                dest_fd = client_a[i].fd;
                aux = 0;
                break;
            }
        }
        if (aux == -1) {
            fprintf(stderr, "\n ### ERROR: [MSG] Dest not found.\n");
            return -1;
        }

        int ret = send_message(dest_fd, msg);
        if (ret < 0) {
            return ret;
        }

        return 0;
    }

    // Send the message for all client connected.
    if (dest == 0) {
        for (int i = 0; i < 10; i++) {
            if (client_a[i].id > 0) {
                int ret = send_message(client_a[i].fd, msg);
                if (ret < 0) {
                    return ret;
                }
            };
        }
        return 0;
    }
    
    fprintf(stderr, "\n ### ERROR: [MSG] Bad.\n");

    return -1;
}

/**
 * @brief Check the time that has passed since the last check.
 *
 * @return Upon succesfull time passed is returned. otherwise
 * a negative error.
 */
time_t check_time (time_t *last_time) {
    time_t current_time; 

    current_time = time(NULL);

    return current_time - *last_time;;
}

/**
 * @brief Send information about the server.
 *
 * @return Upon succesfull time passed is returned. otherwise
 * a negative error.
 */
int send_infoserver(time_t start_time) {

    struct msg_t msg;

    long int uptime = time(NULL) - start_time; 

    msg.type = MSG;
    msg.orig_uid = 0;
    msg.dest_uid = 0;
    sprintf((char *) msg.text,
            "Server info -> Connected Clients: %i, Up time: %li", count_clients, uptime);
    msg.text_len = sizeof(msg.text);

    // Send the message for all client connected.
    for (int i = 0; i < 10; i++) {
        if (client_a[i].id > 0) {
            int ret = send_message(client_a[i].fd, msg);
            if (ret < 0) {
                return ret;
            }
        };
    }
    return 0;
}

/*============================================================================*
 * Main Function                                                              *
 *============================================================================*/

int main(int argc, char *argv[])
{
    int ret = 0;
    int port = 0;
    struct msg_t msg;

    time_t start_time, last_time;
    start_time = last_time = time(NULL);

    // Client array.
    for (int i = 0; i < 20; i++) {
        client_a[i].id = -1;
    }

    fd_set active_fd_set;
    fd_set read_fd_set;

    // Get port from args.
    ret = get_port(argc, argv, &port);
    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Get port failed. errno = %i\n", errno);
        exit(EXIT_FAILURE);
    }

    // Open a socket.
    int sockfd = open_socket();
    if (sockfd < 0) {
        fprintf(stderr, "\n ### ERROR: Open socket failed. errno = %i\n", errno);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Socket = %i \n", sockfd);

    // Naming the socket.
    ret = setaddr_socket(sockfd, port);
    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Naming socket failed. errno = %i\n", errno);
        exit(EXIT_FAILURE);
    }

    // Set listen to the socket.
    ret = listen_socket(sockfd);
    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Listen failed. errno = %i\n", errno);
        exit(EXIT_FAILURE);
    }

    // Initialize set of file descriptors.
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);

    while (1) {
        read_fd_set = active_fd_set;

        // Set select() timeout.
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Wait for a input from active sockets.
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
            fprintf(stderr, "\n ### ERROR: Select failed. errno = %i\n", errno);
            exit(EXIT_FAILURE);
        }

        // Send periodic messages.
        if (check_time(&last_time) >= TIME_INFO) {
            if(send_infoserver(start_time) < 0){
                fprintf(stderr, "\n ### ERROR: Failed send inforserver. errno = %i\n", errno);
                exit(EXIT_FAILURE);
            };
            last_time = time(NULL);
        }

        // Service sockets with arrive data.
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fd_set)) {
                // Connection request.
                if (i == sockfd) {
                    int newsock = accept_connection(sockfd);
                    if (newsock < 0) {
                        fprintf(stderr, "\n ### ERROR: Connection failed. errno = %i\n", errno);
                    } else {
                        fprintf(stdout, "Accept new connection - new socket = %i.\n", newsock);
                        // Add new socket in active fd set.
                        FD_SET(newsock, &active_fd_set);
                    }
                } else {
                    // Receive incomming Message.
                    if (receive_message(i, &msg) <= 0) {
                        // Failled to receive message, disconnect client.
                        goto jp_tchau;
                    }

                    switch (msg.type) {
                        case OI:
                            if (service_oi(msg, i) < 0) {
                                // If OI fail disconnect client.
                                goto jp_tchau;
                            }
                            break;
                        case MSG:
                            if (service_msg(msg) == -1) {
                                // If MSG fail disconnect client.
                                goto jp_tchau;
                            }
                            break;
                        case TCHAU:
                            jp_tchau:
                                service_tchau(i);
                                close_socket(i);
                                FD_CLR (i, &active_fd_set);
                                fprintf(stdout, "Close connection socket: %i.\n", i); 
                            break;
                        default:
                            fprintf(stderr, "\n ### ERROR: Invalid message type.\n");
                    }
                }
            }
        }
    }

    ret = close_socket(sockfd);
    if (ret < 0) {
        return -1;
    }

    return 0;
}
