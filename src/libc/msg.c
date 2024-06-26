/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <msg.h>

/*============================================================================*
 * Public Functions                                                          *
 *============================================================================*/

/**
 * @brief Receive mensage from a socket.
 *
 * @return Upon succesfull the number of bytes read is returned. otherwise
 * a negative error if failed or 0 if zero bytes read.
 */
int receive_message(int sockfd, struct msg_t *msg)
{
    size_t length = sizeof(*msg);
    
    // Read the arrived data.
    int ret = read(sockfd, msg, length);

    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Failed to receive message. errno: %i \n", errno);
        return -1;
    }

    if (ret == 0) {
        fprintf(stderr, "\n ### ERROR: No data read from socket: %i \n", sockfd);
        return -2;
    }
    
    return ret;
}

/**
 * @brief Receive mensage from a socket.
 *
 * @return Upon succesfull the number of bytes read is returned. otherwise
 * a negative error if failed or 0 if zero bytes read.
 */
int send_message(int dest_sock, struct msg_t msg) {

    size_t length = sizeof(msg);

    int ret = write(dest_sock, &msg, length);
    if (ret < 0) {
        fprintf(stderr, "\n ### ERROR: Failed to send %i message. errno: %i \n",msg.type, errno);
        return -1;
    }

    if (ret == 0) {
        fprintf(stderr, "\n ### ERROR: No data sended from socket: %i.\n", dest_sock);
        return -2;
    }

    return ret;
}
