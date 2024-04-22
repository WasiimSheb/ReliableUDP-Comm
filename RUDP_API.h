/*Header file for the Reliable UDP (RUDP) API.*/
#include <stdint.h>   // For uint8_t, uint16_t
#include <stdbool.h>  // For bool type
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#define BUFFER_SIZE 2097152 //Maximum size for sending/receiving data in RUDP.
#define MAX_PACKET_SIZE 8192

typedef struct rudp_flags{
    u_int8_t data;
    u_int8_t done;
    u_int8_t ack;
    u_int8_t syn;
}flags;

typedef struct Rudp_socket{
    u_int16_t len;
    u_int16_t checksum;
    int seqnum;
    char data[MAX_PACKET_SIZE];
    flags flags;
}rudp_sock;


/*in this implementation instead of returning a pointer to the RUDP_Socket itslef we'll return an integer pointing to whether the function succeeded doing it's work*/

// creating an rudp socket, returning -1 if it did not succeed
int rudp_socket();

// Tries to connect to the other side via RUDP to given IP and port. Returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to server.
int rudp_connect(int sockfd, const char *dest_ip, unsigned short int dest_port);

// Accepts incoming connection request and completes the handshake, returns 0 on failure and 1 on success. Fails if called when the socket is connected/set to client.
int rudp_accept(int sockfd, int port);

// Receives data from the other side and put it into the buffer. Returns the number of received bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_recv(int sockfd, void *buffer, unsigned int buffer_size);

// Sends data stores in buffer to the other side. Returns the number of sent bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_send(int sockfd, void *buffer, unsigned int buffer_size);

// Disconnects from an actively connected socket. Returns 1 on success, 0 when the socket is already disconnected (failure).
int rudp_disconnect(int sockfd);

// This function releases all the memory allocation and resources of the socket.
int rudp_close(int sockfd);
