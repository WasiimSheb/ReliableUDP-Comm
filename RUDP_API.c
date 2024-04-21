#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "RUDP_API.h"
#include <sys/time.h>
#define SERVER_PORT 8080
#define CLIENT_PORT 9090
#define SERVER_IP "127.0.0.1" // Change this to the server's IP address
#define CRC32_POLYNOMIAL 0xEDB88320L
#define max_wait_time 2

// Function to create RUDP socket and perform handshake
#include "RUDP_API.h"

int rudp_socket(int port) {
    int sockfd;

    // Create a socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Prepare the server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface
    server_addr.sin_port = htons(port);        // Set the port

    // Bind the socket to the server address
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return -1;
    }

    // Initialize the RUDP socket structure
    Rudp_socket *rudp_sock = (Rudp_socket *)malloc(sizeof(Rudp_socket));
    if (rudp_sock == NULL) {
        perror("memory allocation failed");
        close(sockfd);
        return -1;
    }
    rudp_sock->available = 1;  // Mark the socket as available for connection
    rudp_sock->destinationaddr = server_addr;  // Store the destination address

    // Optionally, perform handshake protocol here if needed
    if (perform_handshake() != 0) {
        perror("handshake failed");
        close(sockfd);
        free(rudp_sock);  // Free allocated memory
        return -1;
    }

    return sockfd;  // Return the socket file descriptor
}

// Function to handle timeout for receiving data
int handle_timeout(int sockfd) {
    // Initialize file descriptor set for select
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    // Initialize timeout struct
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    // Wait for activity on the socket within the timeout period
    int activity = select(sockfd + 1, &fds, NULL, NULL, &timeout);
    if (activity < 0) {
        perror("select failed");
        return -1;
    } else if (activity == 0) {
        // Timeout occurred
        return -1;
    }

    return 0;  // No timeout, data is available
}

/**
 * Function to calculate CRC-32 algorithm checksum for generic data
*/
uint32_t calculate_crc32(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;

    uint32_t crc = 0xFFFFFFFF; 

    for (size_t i = 0; i < size; i++) {
        crc ^= bytes[i]; 

        for (int j = 0; j < 8; j++) { 
            if (crc & 1) {  
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;  
            }
        }
    }

    crc = ~crc;
    return crc;  
}

// Function to with CRC-32 checksum
int send_message_with_checksum(int sockfd, const char *message, size_t size, struct sockaddr_in servaddr) {
    // Calculate CRC-32 checksum
    uint32_t checksum = calculate_crc32((const uint8_t *)message, size);

    // Append checksum to message
    char message_with_checksum[size + sizeof(uint32_t)];
    memcpy(message_with_checksum, message, size);
    memcpy(message_with_checksum + size, &checksum, sizeof(uint32_t));

    // Send message with checksum
    ssize_t bytes_sent = sendto(sockfd, message_with_checksum, size + sizeof(uint32_t), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (bytes_sent < 0) {
        perror("sendto failed");
        return -1;
    }
    return bytes_sent;
}

// Function to receive message with CRC-32 checksum
ssize_t recv_message_with_checksum(int sockfd, char *buffer, size_t size, struct sockaddr_in *src_addr) {
    // Receive message with checksum
    ssize_t bytes_received = recvfrom(sockfd, buffer, size, 0, (struct sockaddr *)src_addr, sizeof(struct sockaddr_in));
    if (bytes_received < 0) {
        perror("recvfrom failed");
        return -1;
    }

    // Extract checksum from received message
    uint32_t received_checksum;
    memcpy(&received_checksum, buffer + bytes_received - sizeof(uint32_t), sizeof(uint32_t));

    // Calculate CRC-32 checksum of message data
    uint32_t computed_checksum = calculate_crc32((const uint8_t *)buffer, bytes_received - sizeof(uint32_t));

    // Compare checksums
    if (computed_checksum != received_checksum) {
        fprintf(stderr, "Checksum mismatch! Data may be corrupted.\n");
        // Handle error (e.g., request retransmission or discard message)
        return -1;
    }

    // Message is intact, return size excluding checksum
    return bytes_received - sizeof(uint32_t);
}


// Function to perform handshake protocol
int perform_handshake() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(SERVER_PORT);

    // Filling client information
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = INADDR_ANY;
    cliaddr.sin_port = htons(CLIENT_PORT);

    // Send connection request to server
    char handshake_msg[] = "Connection request";
    if (sendto(sockfd, handshake_msg, strlen(handshake_msg), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto failed");
        close(sockfd);
        return -1;
    }

    printf("Connection request sent to server.\n");

    // Receive connection acknowledgment from server
    char ack_msg[256];
    socklen_t len = sizeof(servaddr);
    int n = recvfrom(sockfd, ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&servaddr, &len);
    if (n < 0) {
        perror("recvfrom failed");
        close(sockfd);
        return -1;
    }

    ack_msg[n] = '\0';
    printf("Received acknowledgment from server: %s\n", ack_msg);

    close(sockfd);

    return 0;
}


// Function to send a close request to the peer
int rudp_close_request(int sockfd, struct sockaddr_in dest_addr, int dest_port) {
    char close_request_msg[] = "Close request";
    return rudp_send(sockfd, dest_addr, dest_port, close_request_msg, strlen(close_request_msg));
}

// Function to acknowledge the close request
int rudp_close_ack(int sockfd, struct sockaddr_in dest_addr, int dest_port) {
    char close_ack_msg[] = "Close acknowledgment";
    return rudp_send(sockfd, dest_addr, dest_port, close_ack_msg, strlen(close_ack_msg));
}
