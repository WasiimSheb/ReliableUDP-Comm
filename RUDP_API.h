#ifndef RUDP_API_H
#define RUDP_API_H

#include <stdio.h>          // Standard input/output functions
#include <stdlib.h>         // Standard library functions
#include <string.h>         // String manipulation functions
#include <unistd.h>         // POSIX operating system API
#include <sys/socket.h>     // Socket programming functions
#include <netinet/in.h>     // Internet address family structures
#include <arpa/inet.h>      // Functions for manipulating IP addresses
#include <stdbool.h>
#define TIMEOUT_SEC 5
/**
 * rudp packet which contains the information that we'll need to print the statistics of the packet itself
*/
typedef struct rudpp{
    u_int16_t len;
    u_int32_t checksum;
    u_int8_t flags;
    u_int32_t ack;
    u_int16_t seq_n;
    char buffersize[256];
}Rudp_pack;

/**
 * a struct contains two fields which are for the destination address and the other for if there is a connection.
*/
typedef struct rudpsocket{
    int available;
    struct sockaddr_in destinationaddr;
}Rudp_socket;


/**
 * performing the handshake protocol in the rudp socket, where it would return 0 if it succeeded
*/
int perform_handshake();


/**
 * Function in order to calculate the checksum
*/
int checksum();

/**
 * @file RUDP_API.h
 * @brief Header file for the Reliable UDP (RUDP) API.
 */

// Function prototypes for RUDP API

/**
 * @brief Create a RUDP socket and establish a connection with the peer.
 * 
 * @param port The port number to bind the socket.
 * @return The file descriptor of the RUDP socket on success, or -1 on failure.
 */
int rudp_socket(int port);

/**
 * @brief Send data to the peer using RUDP.
 * 
 * @param sockfd The file descriptor of the RUDP socket.
 * @param dest_addr The address of the destination peer.
 * @param dest_port The port number of the destination peer.
 * @param data Pointer to the data to be sent.
 * @param size Size of the data to be sent.
 * @return Number of bytes sent on success, or -1 on failure.
 */
int rudp_send(int sockfd, struct sockaddr_in dest_addr, int dest_port, void *data, int size);

/**
 * @brief Receive data from the peer using RUDP.
 * 
 * @param sockfd The file descriptor of the RUDP socket.
 * @param src_addr Pointer to store the source address of the received data.
 * @param src_port Pointer to store the source port of the received data.
 * @param buffer Buffer to store the received data.
 * @param size Size of the buffer.
 * @return Number of bytes received on success, or -1 on failure.
 */
int rudp_recv(int sockfd, struct sockaddr_in *src_addr, int *src_port, void *buffer, int size);

/**
 * @brief Close the RUDP connection.
 * 
 * @param sockfd The file descriptor of the RUDP socket to be closed.
 * @return 0 on success, or -1 on failure.
 */
int rudp_close(int sockfd);

/**
 * Function to send ACK to sender
 * */ 
int send_ack(int sockfd, struct sockaddr_in dest_addr, int dest_port);

/**
 * Function to handle the ACK from the server
*/
int handle_ack(int sockfd);

#endif /* RUDP_API_H */
