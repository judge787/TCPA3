#include <stdio.h>              // For standard input/output functions like printf()
#include <errno.h>              // For error codes and strerror()
#include <ctype.h>              // For character manipulation functions like toupper()
#include <stdlib.h>             // For general-purpose functions like exit()
#include <string.h>             // For string handling functions like memset()
#include <unistd.h>             // For POSIX functions like close()
#include <signal.h>             // For handling signals like SIGINT
#include <arpa/inet.h>          // For internet operations like inet_ntoa()
#include <sys/types.h>          // For data types used in socket programming
#include <netinet/in.h>         // For structures and macros related to the Internet Protocol
#include <sys/socket.h>         // For socket programming functions and definitions

#define PORTNUM 50555           // Define the port number on which the server listens
#define MAXRCVLEN 500           // Define the maximum length of a received message

int mysocket;                   // Global variable for the main server socket
int consocket;                  // Global variable for the connection socket with a client

// Signal handler for catching SIGINT (Ctrl+C) and shutting down the server gracefully
static void sigintCatcher(int signal, siginfo_t* si, void *arg) {
    
	printf("\n\n************** Caught SIG_INT: shutting down the server ********************\n");

    // Close both the connection socket and the server socket
    close(consocket);
    close(mysocket);

    // Exit the program
    exit(0);
}

int main(int argc, char *argv[]) {
    int len;                    // Variable to store the length of the received message
    char buffer[MAXRCVLEN + 1]; // Buffer to store received data (+1 for null terminator)
    struct sockaddr_in dest;    // Structure to hold client's address information
    struct sockaddr_in serv;    // Structure to hold server's address information

    // Setting up the signal handler for SIGINT (e.g., Ctrl+C)
    struct sigaction signaler;    
    memset(&signaler, 0, sizeof(struct sigaction)); // Zero out the signal action structure
    sigemptyset(&signaler.sa_mask);                // Initialize the signal mask

    // Specify the handler function and flags
    signaler.sa_sigaction = sigintCatcher;         
    signaler.sa_flags = SA_SIGINFO;                // Use the extended signal handler
    sigaction(SIGINT, &signaler, NULL);            // Register the signal handler for SIGINT

    // Initialize the server's address structure
    socklen_t socksize = sizeof(struct sockaddr_in); // Size of the sockaddr_in structure
    memset(&serv, 0, sizeof(serv));                 // Zero out the server's address structure
    serv.sin_family = AF_INET;                      // Set the address family to IPv4
    serv.sin_addr.s_addr = htonl(INADDR_ANY);       // Accept connections on any available network interface
    serv.sin_port = htons(PORTNUM);                 // Set the server's port number in network byte order

    // Create a socket for the server
    mysocket = socket(AF_INET, SOCK_STREAM, 0);     // Create an IPv4, TCP socket

    // Allow the server to reuse the address if it is in TIME_WAIT state
    int flag = 1;
    if (setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
        printf("setsockopt() failed\n");
        printf("%s\n", strerror(errno));            // Print the error message
        exit(1);                                    // Exit on failure
    }

    // Bind the server's address information to the socket
    if (bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) != 0) {
        printf("Unable to open TCP socket on localhost:%d\n", PORTNUM);
        printf("%s\n", strerror(errno));            // Print the error message
        close(mysocket);                            // Close the socket
        return 0;                                   // Exit the program
    }

    // Start listening for incoming connections with a queue size of 1
    listen(mysocket, 0);

    // Accept a connection from a client
    consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);

    // Main loop: Handle client connections
    while (consocket) {
        // Print details about the incoming connection
        printf("Incoming connection from %s on port %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

        // Receive data from the client
        len = recv(consocket, buffer, MAXRCVLEN, 0);
        buffer[len] = '\0'; // Null-terminate the received data
        printf("Received %s\n", buffer);

        // Convert the received message to uppercase
        int bufLen = strlen(buffer);
        for (int i = 0; i < bufLen; i++) {
            buffer[i] = toupper(buffer[i]);
        }

        // Send the modified data back to the client
        send(consocket, buffer, strlen(buffer), 0);

        // Close the current connection
        close(consocket);

        // Continue listening for more incoming connections
        consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
    }

    // Close the server socket
    close(mysocket);
    return EXIT_SUCCESS; // Indicate successful program execution
}
