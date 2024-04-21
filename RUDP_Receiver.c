
#include <stdio.h>          // Standard input/output functions
#include <stdlib.h>         // Standard library functions
#include <string.h>         // String manipulation functions
#include <unistd.h>         // POSIX operating system API
#include <sys/socket.h>     // Socket programming functions
#include <netinet/in.h>     // Internet address family structures
#include <arpa/inet.h>      // Functions for manipulating IP addresses

#define MAX_BUF_SIZE 1024
#include "RUDP_API.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    // Create RUDP socket
    int sockfd = rudp_socket(port);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create RUDP socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sender_addr;
    char buffer[MAX_BUF_SIZE];

    while (1) {
        // Receive data from sender using RUDP
        struct sockaddr_in src_addr;
        int src_port;
        ssize_t bytes_received = rudp_recv(sockfd, &src_addr, &src_port, buffer, sizeof(buffer));
        if (bytes_received < 0) {
            fprintf(stderr, "Failed to receive data using RUDP\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Received message from sender: %s\n", buffer);

        // Process received message (optional)
        // ...

        // Send ACK to sender
        if (send_ack(sockfd, src_addr, src_port) < 0) {
            fprintf(stderr, "Failed to send ACK\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    // Close RUDP socket (will never reach here in this example)
    if (rudp_close(sockfd) < 0) {
        fprintf(stderr, "Failed to close RUDP socket\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
