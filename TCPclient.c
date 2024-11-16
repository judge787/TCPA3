#include <stdio.h>              // For standard input/output functions (e.g., printf, scanf)
#include <stdlib.h>             // For general-purpose functions (e.g., exit, malloc)
#include <string.h>             // For string handling functions (e.g., memset, strlen)
#include <unistd.h>             // For close() and other POSIX functions
#include <arpa/inet.h>          // For definitions for internet operations (e.g., inet_aton)
#include <sys/types.h>          // For data types used in socket programming
#include <netinet/in.h>         // For structures and macros related to the Internet Protocol
#include <sys/socket.h>         // For socket programming functions and definitions

#define MAXRCVLEN 500           // Define the maximum length of a received message
#define PORTNUM 50555           // Define the port number to connect to

int main(int argc, char *argv[]) {
    char buffer[MAXRCVLEN + 1]; /* Buffer to store the received data (+1 for null terminator) */
    char* msg = "Hello Worlds!\n"; /* The message that will be sent to the server */
    int len, mysocket;          // Length of received message and the socket file descriptor
    struct sockaddr_in dest;    // Structure to hold server's address information
    struct sockaddr_in src;     // Structure to hold client's address information

    /* Create a socket.
       - AF_INET: Specifies IPv4 address family.
       - SOCK_STREAM: Specifies TCP (reliable, connection-oriented).
       - 0: Specifies default protocol for SOCK_STREAM (TCP).
       Returns a file descriptor or -1 if socket creation fails.
    */
    mysocket = socket(AF_INET, SOCK_STREAM, 0);
  
    // Clear the dest and src structures to remove garbage values
    memset(&dest, 0, sizeof(dest)); 
    memset(&src, 0, sizeof(src));

    // Set up the destination server's address details
    dest.sin_family = AF_INET;  // Use the IPv4 address family

    /* Determine the IP address of the destination server.
       - If no arguments are provided, use localhost (127.0.0.1).
       - If one argument is provided, use the given IP address.
       - If more than one argument is provided, print usage instructions and exit.
    */
    if (argc == 1) {
        dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Convert localhost (127.0.0.1) to network byte order
    } else if (argc == 2) {
        inet_aton(argv[1], &dest.sin_addr); // Convert provided IP address to network format
    } else {
        printf("Invalid number of arguments\n");
        printf("Usage: %s <ip>, where ip is an optional IPv4 address in dot-decimal notation\n", argv[0]);
        printf("If the IP address is not provided, the client connects to localhost\n");
        exit(0);  // Exit the program with an error state
    }

    dest.sin_port = htons(PORTNUM);  // Set the port number (convert to network byte order)

    /* Connect to the server.
       - mysocket: The socket file descriptor created earlier.
       - dest: Pointer to the destination server's address structure.
       - sizeof(struct sockaddr_in): Size of the server's address structure.
    */
    connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));

    /* Get the details of the local (client) side of the connection.
       - src: Structure to store client's socket details.
       - sLen: Variable to store the size of the structure.
    */
    socklen_t sLen = sizeof(src);
    getsockname(mysocket, (struct sockaddr *) &src, &sLen);

    // Print connection details: local IP/port and server IP/port
    printf("Outgoing connection from %s on port %d\n", inet_ntoa(src.sin_addr), ntohs(src.sin_port));
    printf("Outgoing connection to %s on port %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
  
    /* Send a message to the server.
       - mysocket: The socket connected to the server.
       - msg: The message to send.
       - strlen(msg): Length of the message.
       - 0: Flags (no special options).
    */
    send(mysocket, msg, strlen(msg), 0); 

    /* Receive data from the server.
       - mysocket: The socket connected to the server.
       - buffer: Buffer to store the received data.
       - MAXRCVLEN: Maximum number of bytes to read.
       - 0: Flags (no special options).
       Returns the number of bytes received.
    */
    len = recv(mysocket, buffer, MAXRCVLEN, 0);
  
    /* Null-terminate the received data to make it a valid string */
    buffer[len] = '\0';
  
    // Print the received message and its details
    printf("Received %s (%d bytes) from %s on port %d.\n", buffer, len, inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

    /* Close the socket to release resources */
    close(mysocket);

    return EXIT_SUCCESS;  // Indicate successful execution
}

