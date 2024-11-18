#include <stdio.h>        // For input/output functions like printf()
#include <stdlib.h>       // For general-purpose functions like atoi()
#include <string.h>       // For string handling functions like memset()
#include <unistd.h>       // For POSIX functions like close()
#include <arpa/inet.h>    // For internet operations like inet_pton()

#define DEFAULT_BUF_SIZE 4096 // Default buffer size for reading file data

int main(int argc, char *argv[]) {
    // Check if correct number of arguments is provided
    if (argc < 3) {
        printf("Usage: %s <fileName> <IP-address>:<port-number> [bufSize]\n", argv[0]); // Print usage information
        return 1; // Exit the program with an error code
    }

    char *fileName = argv[1]; // Get the file name from command line arguments
    char *ipPort = argv[2]; // Get the IP address and port number from command line arguments
    int bufSize = (argc == 4) ? atoi(argv[3]) : DEFAULT_BUF_SIZE; // Set buffer size from arguments or use default

    // Split the IP address and port number
    char *colon = strchr(ipPort, ':'); // Find the colon that separates IP and port
    if (!colon) { // Check if colon was found
        printf("Invalid IP:Port format\n"); // Print an error message if format is invalid
        return 1; // Exit the program with an error code
    }
    *colon = '\0'; // Replace the colon with a null terminator to separate IP and port
    char *ipAddress = ipPort; // IP address is before the colon
    int port = atoi(colon + 1); // Port number is after the colon

    // Open the file to read
    FILE *file = fopen(fileName, "rb"); // Open the file in binary read mode
    if (!file) { // Check if the file was successfully opened
        perror("Error opening file"); // Print an error message if failed
        return 1; // Exit the program with an error code
    }

    // Create a TCP socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0); // Create an IPv4, TCP socket
    if (clientSocket < 0) { // Check if the socket creation was successful
        perror("Error creating socket"); // Print an error message if failed
        fclose(file); // Close the file
        return 1; // Exit the program with an error code
    }

    // Set up the server address structure
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // Zero out the server address structure
    serverAddr.sin_family = AF_INET; // Set the address family to IPv4
    serverAddr.sin_port = htons(port); // Set the server port number in network byte order
    if (inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr) <= 0) { // Convert IP address to binary format
        perror("Invalid IP address"); // Print an error message if IP conversion fails
        close(clientSocket); // Close the socket
        fclose(file); // Close the file
        return 1; // Exit the program with an error code
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) { // Attempt connection
        perror("Connection failed"); // Print an error message if connection fails
        close(clientSocket); // Close the socket
        fclose(file); // Close the file
        return 1; // Exit the program with an error code
    }

    // Send the file name to the server
    send(clientSocket, fileName, strlen(fileName) + 1, 0); // Send file name (including null terminator)

    // Send the file contents
    char *buffer = malloc(bufSize); // Allocate memory for the buffer
    size_t bytesRead; // Variable to hold the number of bytes read
    while ((bytesRead = fread(buffer, 1, bufSize, file)) > 0) { // Read data from file in chunks
        // send(clientSocket, buffer, bytesRead, 0); // Send the data to the server
        uint32_t chunkSize = htonl(bytesRead); // Convert chunk size to network byte order
        send(clientSocket, &chunkSize, sizeof(chunkSize), 0); // Send chunk size
        send(clientSocket, buffer, bytesRead, 0); // Send chunk data

        printf("Sent chunk of size %zu bytes.\n", bytesRead); // Debug log
    }

    // Clean up resources
    free(buffer); // Free the allocated buffer memory
    fclose(file); // Close the file
    close(clientSocket); // Close the client socket
    printf("File '%s' sent successfully\n", fileName); // Print a success message
    return 0; // Exit the program successfully
}
