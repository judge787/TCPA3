#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_BUF_SIZE 4096 // Default buffer size

void handle_client(int client_socket, char *buf, int buf_size) {
    char file_name[256]; // Buffer to store the name of the file being received from the client

    // Receive the file name from the client
    int received = recv(client_socket, file_name, sizeof(file_name), 0); // Read the file name sent by the client
    if (received <= 0) { // Check if the file name was successfully received
        perror("Failed to receive file name"); // Print an error message if the file name was not received
        close(client_socket); // Close the connection with the client
        return; // Exit the function as there is no valid file name
    }

    // Handle duplicate file names to prevent overwriting existing files
    char saved_file[512]; // Buffer to store the final file path (handles duplicates if necessary)
    snprintf(saved_file, sizeof(saved_file), "./%s", file_name); // Initialize the file path with the received file name

    int suffix = 1; // Suffix to append to the file name in case of duplicates (e.g., "file(1).txt")
    while (access(saved_file, F_OK) == 0) { // Check if a file with the same name already exists
        snprintf(saved_file, sizeof(saved_file), "./%s(%d)", file_name, suffix++); // Modify the file name by appending a suffix
    }

    // Open the file for binary writing
    FILE *file = fopen(saved_file, "wb");
    if (!file) {
        perror("Failed to create file");
        close(client_socket);
        return;
    }

    uint32_t chunk_size;
    ssize_t bytes = 0;        // Bytes received in each chunk
    size_t total_bytes = 0; // Total size of the file received

    // Start receiving the file data
    // while ((bytes = recv(client_socket, buf, buf_size, 0)) > 0) { // Receive chunks of data
    //     size_t bytes_written = fwrite(buf, 1, (size_t)bytes, file); // Write received data to the file
    //     if (bytes_written != (size_t)bytes) { 
    //         perror("Error writing to file"); // Check if fwrite wrote all bytes successfully
    //         break; // Stop the loop if there is a write error
    //     }

    //     total_bytes += bytes_written; // Update the cumulative total bytes received

    //     // Debug log to show the size of the chunk received and written
    //     printf("Received %zd bytes in this chunk, wrote %zu bytes. Total so far: %zu bytes.\n", bytes, bytes_written, total_bytes);
    // }
     ssize_t bytes_received;
     while ((bytes_received = recv(client_socket, &chunk_size, sizeof(chunk_size), 0)) > 0) {
        chunk_size = ntohl(chunk_size); // Convert chunk size to host byte order

        printf("Expecting chunk of size %u bytes.\n", chunk_size); // Debug log

        size_t received_so_far = 0;
        while (received_so_far < chunk_size) {
            bytes_received = recv(client_socket, buf, (size_t)buf_size < (chunk_size - received_so_far) ? (size_t)buf_size : (chunk_size - received_so_far), 0);
            if (bytes_received <= 0) {
                perror("Error while receiving file chunk");
                fclose(file);
                close(client_socket);
                return;
            }

            fwrite(buf, 1, bytes_received, file); // Write the received data to the file
            received_so_far += bytes_received; // Track the number of bytes received
            total_bytes += bytes_received;

            printf("Received %zd bytes in this chunk. Total received for this chunk: %zu/%u bytes.\n", bytes_received, received_so_far, chunk_size);
        }

        printf("Chunk of size %u bytes received successfully.\n", chunk_size);
    }

    // Handle errors or end of transmission
    if (bytes < 0) { 
        // Log an error if the recv() call fails
        perror("Error while receiving file data");
    } else {
        // Log the successful completion of file transfer
        printf("File '%s' saved successfully.\n", saved_file);
        printf("File size: %zu bytes\n", total_bytes); // Display the total size of the file received
    }

    // Clean up resources
    fclose(file); // Close the file after writing all data
    close(client_socket); // Close the client socket connection
}

int main(int argc, char *argv[]) {
    // Check if the required arguments (port number and optional buffer size) are provided
    if (argc < 3) {
        printf("Usage: %s <port-number> [bufSize]\n", argv[0]); // Print usage instructions
        return 1; // Exit the program with an error code
    }

    // Get the port number from the command-line arguments
    int port = atoi(argv[1]); // Convert the first argument to an integer (port number)
    
    // Get the buffer size from the command-line arguments, or use the default value
   size_t buf_size = (argc == 3) ? atoi(argv[2]) : DEFAULT_BUF_SIZE; // Use size_t for buffer size


    // Allocate memory for the buffer used to receive data
    char *buf = malloc(buf_size); // Dynamically allocate buffer memory
    if (!buf) { // Check if memory allocation was successful
        perror("Failed to allocate buffer"); // Print an error message if allocation fails
        return 1; // Exit the program with an error code
    }

    // Create a socket for the server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0); // Create an IPv4 TCP socket
    if (server_socket < 0) { // Check if socket creation was successful
        perror("Failed to create socket"); // Print an error message if socket creation fails
        free(buf); // Free allocated memory
        return 1; // Exit the program with an error code
    }

    // Define the server's address structure
    struct sockaddr_in server_addr; // Structure to hold server address information
    memset(&server_addr, 0, sizeof(server_addr)); // Initialize the structure to zero
    server_addr.sin_family = AF_INET; // Set address family to IPv4
    server_addr.sin_port = htons(port); // Set the server port (convert to network byte order)
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address

    // Bind the socket to the specified address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket"); // Print an error message if binding fails
        close(server_socket); // Close the socket
        free(buf); // Free allocated memory
        return 1; // Exit the program with an error code
    }

    // Start listening for incoming connections with a backlog of 5
    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen on socket"); // Print an error message if listening fails
        close(server_socket); // Close the socket
        free(buf); // Free allocated memory
        return 1; // Exit the program with an error code
    }

    // Print a message indicating that the server is ready to accept connections
    printf("Server listening on port %d with buffer size %zu bytes.\n", port, buf_size);

    // Loop to continuously accept and handle client connections
    while (1) {
        struct sockaddr_in client_addr; // Structure to hold client address information
        socklen_t addr_len = sizeof(client_addr); // Length of the client address structure

        // Accept an incoming connection from a client
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) { // Check if the connection was successfully accepted
            perror("Failed to accept connection"); // Print an error message if connection fails
            continue; // Continue to the next iteration to accept the next connection
        }

        // Log the client's IP address and port number
        printf("Connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Log the buffer size being used by the server
        printf("Buffer size: %zu bytes\n", buf_size);

        // Call the function to handle the client's request
        handle_client(client_socket, buf, buf_size);
    }

    // Close the server socket and free allocated resources (not reached in this code)
    close(server_socket); // Close the server socket
    free(buf); // Free the allocated buffer memory
    return 0; // Exit the program successfully
}

