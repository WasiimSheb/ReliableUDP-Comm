
#include <stdio.h>     // Standard input-output functions
#include <stdlib.h>    // Standard library functions
#include <string.h>    // String manipulation functions
#include <unistd.h>    // POSIX operating system API
#include <sys/socket.h> // Socket programming functions and structures
#include <arpa/inet.h>  // Definitions for internet operations
#include <netinet/tcp.h> // Definitions for TCP/IP protocol
#include <stdbool.h>     // Standard boolean type and values
#include <sys/time.h>    // Time-related functions and structures
#include <time.h> 
#include "RUDP_API.h"

#define MAX_PACKET_SIZE 8192
#define BUFFER_SIZE 2097152
#define SERVER_IP "127.hy0.0.1"
#define MAX_WAIT_TIME 3

// implementation of how to calculate the checksum
unsigned short int calculate_checksum(void *data, unsigned int bytes) { 
    unsigned short int *data_pointer = (unsigned short int *)data; 
    unsigned int total_sum = 0; 
 
    // Main summing loop 
    while (bytes > 1) { 
        total_sum += *data_pointer++; 
        bytes -= 2; 
    } 
 
    // Add left-over byte, if any 
    if (bytes > 0) 
        total_sum += *((unsigned char *)data_pointer); 
 
    // Fold 32-bit sum to 16 bits 
    while (total_sum >> 16) 
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16); 
 
    return (~((unsigned short int)total_sum)); 
}


int rudp_socket(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1){
        printf(stderr, "error creating a socket from the very beginning");
        return -1;
    }
    return sockfd;
}

int rudp_connect(int sockfd, const char *dest_ip, unsigned short int dest_port) {
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Error setting the timeout :/");
        return -1;
    }

    struct sockaddr_in sv;
    memset(&sv, 0, sizeof(sv));
    sv.sin_family = AF_INET; // IPV4
    sv.sin_port = htons(dest_port);

    // Checking whether the destination IP is valid
    if (inet_addr(dest_ip) == -1) {
        fprintf(stderr, "Error in IP");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&sv, sizeof(sv)) == -1) {
        fprintf(stderr, "Could not establish connection");
        return -1;
    }

    rudp_sock *rudp = (rudp_sock*)malloc(sizeof(rudp_sock));
    if (rudp == NULL) {
        fprintf(stderr, "Error allocating memory for RUDP socket");
        return -1;
    }

    memset(rudp, 0, sizeof(rudp));
    rudp->flags.ack = 1;

    int retry = 0;
    while (retry < 5) {
        if (sendto(sockfd, rudp, sizeof(rudp), 0, NULL, 0) == -1) {
            fprintf(stderr, "Error in sending!");
            free(rudp);
            return -1; // Indicating an error which occurred
        }

        double s = clock();
        while ((s - clock()) < 1) {
            rudp_sock *pack = (rudp_sock*)malloc(sizeof(rudp_sock));
            if (pack == NULL) {
                fprintf(stderr, "Error in allocating memory for receiving");
                free(rudp);
                return -1;
            }

            memset(pack, 0, sizeof(rudp_sock));

            if (recvfrom(sockfd, pack, sizeof(rudp_sock), 0, NULL, 0) == -1) {
                fprintf(stderr, "Error in receiving the data");
                free(pack);
                free(rudp);
                return -1;
            }
            retry++;
            if (pack->flags.ack == 1 && pack->flags.syn == 1) {
                free(rudp);
                free(pack);
                return 1;
            } else {
                fprintf(stderr, "Error packets in connection");
            }
            retry++;
        }
        fprintf(stderr, "No connection");
        free(rudp);
        return 0;
    }
}

// Accepts incoming connection request and completes the handshake, returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to client.
int rudp_accept(int sockfd, int port) {
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return -1;
    }

    socklen_t addr_len = sizeof(client_addr);

    // Receive data from the client
    rudp_sock received_packet;
    ssize_t recv_len = recvfrom(sockfd, &received_packet, sizeof(received_packet), 0, (struct sockaddr *)&client_addr, &addr_len);
    if (recv_len < 0) {
        perror("recvfrom");
        return -1;
    }

    // Check if the received packet contains SYN flag
    if (received_packet.flags.syn != 1) {
        fprintf(stderr, "Received packet does not contain SYN flag\n");
        return -1;
    }

    // Connect to the client
    if (connect(sockfd, (struct sockaddr *)&client_addr, addr_len) < 0) {
        perror("connect");
        return -1;
    }

    // Send SYN-ACK response to the client
    rudp_sock syn_ack_packet;
    memset(&syn_ack_packet, 0, sizeof(syn_ack_packet));
    syn_ack_packet.flags.syn = 1;
    syn_ack_packet.flags.ack = 1;

    ssize_t sent_len = sendto(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, (struct sockaddr *)&client_addr, addr_len);
    if (sent_len < 0) {
        perror("sendto");
        return -1;
    }

    // Set timeout for receiving subsequent packets
    struct timeval timeout;
    timeout.tv_sec = 10; // 10 seconds timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        return -1;
    }

    return 1; // Connection established successfully
}

unsigned short int calculate_checksum(rudp_sock *rudp) {
    unsigned int sum = 0;
    unsigned short *buf = (unsigned short *)rudp;
    unsigned short checksum;

    // Calculate the sum of all 16-bit words
    for (int i = 0; i < sizeof(rudp_sock) / 2; i++) {
        sum += *buf++;
    }

    // Add carry bits to the sum
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take the one's complement
    checksum = ~sum;
    return checksum;
}

int send_ack(int sockfd, rudp_sock *received_packet){
  // Allocate memory for the acknowledgment packet
    rudp_sock *ack_packet = malloc(sizeof(rudp_sock));
    if (ack_packet == NULL) {
        perror("Failed to allocate memory for acknowledgment packet");
        return -1;  // Error occurred
    }
  // Initialize acknowledgment packet flags
    if (received_packet->flags.data == 1) {
        ack_packet->flags.data = 1;  // Indicates a data packet
    }
  // Set acknowledgment packet flags based on the received RUDP packet
    if (received_packet->flags.done == 1) {
        ack_packet->flags.done = 1;  // Indicates completion of data transmission
    }
   // Set the sequence number and calculate the checksum
    ack_packet->seqnum = received_packet->seqnum;
    ack_packet->checksum = calculate_checksum(ack_packet, sizeof(ack_packet));

    // Transmit the acknowledgment packet
    int send_result = sendto(socket, ack_packet, sizeof(rudp_sock), 0, NULL, 0);
    if (send_result == -1) {
        perror("Error: transmit acknowledgment");
        free(ack_packet);
        return -1;  // Transmission error
    }

    // Free memory allocated for the acknowledgment packet
    free(ack_packet);

    // Return success
    return 1;  
}

int wait_ack(int sockfd, int seq_num, clock_t st, clock_t tout) {
    // Allocate memory for the acknowledgment packet
    rudp_sock *ack_packet = malloc(sizeof(rudp_sock));
    if (ack_packet == NULL) {
        perror("Failed to allocate memory for acknowledgment packet");
        return -1;  // Error occurred
    }

    // Loop until timeout
    clock_t elapsed_time;
    double elapsed_seconds;
    do {
        elapsed_time = clock() - st;
        elapsed_seconds = (double)elapsed_time / CLOCKS_PER_SEC;

        // Receive packet from the socket
        int length = recvfrom(sockfd, ack_packet, sizeof(rudp_sock), 0, NULL, 0);
        if (length == -1) {
            perror("Failed to receive acknowledgment packet");
            free(ack_packet);
            return -1;  // Error occurred
        }

        // Check if the received packet matches the expected sequence number and is an acknowledgment
        if (ack_packet->seqnum == seq_num && ack_packet->flags.ack == 1) {
            free(ack_packet);
            return 1;  // Acknowledgment received
        }
    } while (elapsed_seconds < tout);

    free(ack_packet);
    return 0;  // Timeout reached without acknowledgment
}

int rudp_send(int socket, const char *data, int size) {
    // Calculate the number of packets needed to send the data
    int number_packets = size / MAX_PACKET_SIZE;
    int last_packet_size = size % MAX_PACKET_SIZE;

    // Allocate memory for the RUDP packet
    rudp_sock *rudp = malloc(sizeof(rudp_sock));
    if (rudp == NULL) {
        perror("Memory allocation failed");
        return -1;
    }

    // Iterate through each packet to send
    for (int i = 0; i < number_packets; i++) {
        // Initialize the RUDP packet
        memset(rudp, 0, sizeof(rudp));
        rudp->seqnum = i;        // Set the sequence number
        rudp->flags.data = 1;  // Set the data packet flag
        if (i == last_packet_size - 1 && last_packet_size == 0) {
            rudp->flags.done = 1;  // Set the finish flag for the last packet
        }
        // Copy the data into the packet
        memcpy(rudp->data, data + (i * MAX_PACKET_SIZE), MAX_PACKET_SIZE);
        rudp->len = MAX_PACKET_SIZE;  // Set the data length
        rudp->checksum = calculate_checksum(rudp, sizeof(rudp));  // Calculate and set the checksum

        // Send the packet and wait for acknowledgement
        do {
            int res = sendto(socket, rudp, sizeof(rudp_sock), 0, NULL, 0);
            if (res == -1) {
                perror("Error sending with sendto");
                free(rudp);
                return -1;
            }
        } while (wait_for_acknowledgement(socket, i, clock(), 1) <= 0);
    }

    // Send the last packet if there's remaining data
    if (last_packet_size > 0) {
        // Initialize the last packet
        memset(rudp, 0, sizeof(rudp_sock));
        rudp->seqnum = number_packets;  // Set the sequence number
        rudp->flags.data = 1;         // Set the data packet flag
        rudp->flags.done = 1;            // Set the finish flag

        // Copy the last portion of data into the packet
        memcpy(rudp->data, data + number_packets * MAX_PACKET_SIZE, last_packet_size);
        rudp->len = last_packet_size;  // Set the data length
        rudp->checksum = checksum(rudp);    // Calculate and set the checksum

        // Send the last packet and wait for acknowledgement
        do {
            int send_last_packet = sendto(socket, rudp, sizeof(rudp_sock), 0, NULL, 0);
            if (send_last_packet == -1) {
                perror("Error : can't send the last packet");
                free(rudp);
                return -1;
            }
        } while (wait_ack(socket, number_packets, clock(), 1) <= 0);
    }

    // Free the allocated memory for the RUDP packet
    free(rudp);

    return 1;  // Return 1 on successful transmission
}

int seq_number = 0;
int rudp_receive(int socket, char **buffer, int *size){
    // Allocate memory for the received RUDP packet
    rudp_sock *rudp = malloc(sizeof(rudp_sock));
     if (rudp == NULL) {
        perror("Memory allocation failed");
        return -1;
    }
    // Initialize the memory block to zero
    memset(rudp, 0, sizeof(rudp_sock));

    // Receive data from the socket into the allocated memory
    int recv_data = recvfrom(socket, rudp, sizeof(rudp_sock) - 1, 0, NULL, 0);
    if (recv_data == -1) {  
        // Handle the case when an error occurs during receiving
        perror("Error: failed recieving data");
        free(rudp);
        return -1; 
    }

    // Verify the checksum to ensure data integrity
    if (calculate_checksum(rudp, sizeof(rudp)) != rudp->checksum) {  
        free(rudp);
        return -1;
    }

    // Send acknowledgment for the received packet
    if (send_ack(socket, rudp) == -1) {
        free(rudp);
        return -1;
    }

    // Handle connection request
    if (rudp->flags.syn == 1) {  
        printf("there is a new request for connection\n");
        free(rudp);
        return 0;  // Connection request received
    }

    // Check if the received packet is expected based on sequence number
    if (rudp->seqnum == seq_number) {
        // Handle regular data packet
        if (rudp->flags.data == 1 && rudp->seqnum == 0 ) {
            set_time(socket, 1 * 10);
        }
        // Handle the case when the entire message is received and the connection is closed
        if (rudp->flags.done == 1 && rudp->flags.done == 1) {  
            *buffer = malloc(rudp->len);
            if (*buffer == NULL) {
            perror("Failed to allocate memory for the buffer");
            free(rudp);
            return -1;  // Error: Memory allocation failed
         }
             // Copy the received data into the buffer
            memcpy(*buffer, rudp->data, rudp->len);
            *size = rudp->len;
            free(rudp);
            seq_number = 0; 
             struct timeval timeout;
            timeout.tv_sec = 100000; // 10 seconds timeout
            timeout.tv_usec = 0;
            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt");
            return -1;
        }
            return 5;  // Message received and connection closed
        }

        // Handle regular data packet
        if (rudp->flags.data == 1) {  
            *buffer = malloc(rudp->len);
              if (*buffer == NULL) {
              perror("Failed to allocate memory for the buffer");
              free(rudp);
              return -1;  // Error: Mermory allocation failed
    }
            memcpy(*buffer, rudp->data, rudp->len);
            *size = rudp->len; 
            free(rudp);
            seq_number++;  
            return 1;  // Regular data packet received
        }
    }
    // Handle unexpected or out-of-order data packet
    else if (rudp->flags.data == 1) {
        free(rudp);
        return 0;  // Unexpected or out-of-order data packet
    }

    // Handle closing connection
    if (rudp->flags.done == 1) {  
        // Process closing connection
        free(rudp);
        printf("received close connection\n");

        struct timeval timeout;
        timeout.tv_sec = 10; // 10 seconds timeout
        timeout.tv_usec = 0;
        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        return -1;
        rudp = malloc(sizeof(rudp_sock));
        time_t finishing= time(NULL);


        // Wait for acknowledgement for the closing packet
        while ((double)(time(NULL) - finishing) < 1) {
            memset(rudp, 0, sizeof(rudp_sock));
            recvfrom(socket, rudp, sizeof(rudp_sock) - 1, 0, NULL, 0);
            if (rudp->flags.done == 1) {
                if (send_acknowledgement(socket, rudp) == -1) { 
                    free(rudp);
                    return -1;  // Error sending acknowledgment for closing packet
                }
                finishing = time(NULL);
            }
        }

        free(rudp);
        close(socket);
        return -5;  // Connection closed
    }
    free(rudp);
    return 0;  // No data received or unexpected packet type
}
