#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_BUF_SIZE 4096 // Default buffer size

void handle_client(int client_socket, char *buf, int buf_size) {
    memset(buf, 0, buf_size); // Clear buffer at the start of handling a client

    char file_name[256]; // Buffer to store the name of the file being received from the client

    // Receive the file name from the client
    int received = recv(client_socket, file_name, sizeof(file_name), 0);
    if (received <= 0) {
        perror("Failed to receive file name");
        close(client_socket);
        return;
    }

    // Send acknowledgment to the client
    const char *ready_msg = "READY";
    if (send(client_socket, ready_msg, strlen(ready_msg) + 1, 0) == -1) {
        perror("Failed to send acknowledgment to client");
        close(client_socket);
        return;
    }
    printf("Acknowledgment sent to client. Ready to receive file.\n");

    // Handle duplicate file names to prevent overwriting existing files
    char saved_file[512]; // Buffer to store the final file path
    snprintf(saved_file, sizeof(saved_file), "./%s", file_name);

    int suffix = 1; // Suffix for duplicate files
    while (access(saved_file, F_OK) == 0) {
        snprintf(saved_file, sizeof(saved_file), "./%s(%d)", file_name, suffix++);
    }

    // Open the file for binary writing
    FILE *file = fopen(saved_file, "wb");
    if (!file) {
        perror("Failed to create file");
        close(client_socket);
        return;
    }

    uint32_t chunk_size;
    ssize_t bytes_received = 0; // Bytes received in each chunk
    size_t total_bytes = 0;     // Total size of the file received

    // Start receiving file data
    while ((bytes_received = recv(client_socket, &chunk_size, sizeof(chunk_size), 0)) > 0) {
        chunk_size = ntohl(chunk_size); // Convert chunk size to host byte order
        // printf("Expecting chunk of size %u bytes.\n", chunk_size);

        // if (chunk_size == 0 || chunk_size > (uint32_t)buf_size) {
        //     fprintf(stderr, "Invalid chunk size received: %u bytes. Closing connection.\n", chunk_size);
        //     fclose(file);
        //     close(client_socket);
        //     return;
        // }

        size_t received_so_far = 0;
        while (received_so_far < chunk_size) {
            // Declare `len` to track bytes received in each call
            ssize_t len = recv(client_socket, buf,
                               (chunk_size - received_so_far > (size_t)buf_size)
                                ? (size_t)buf_size
                                : (chunk_size - received_so_far),
                               0);
            if (len <= 0) {
                perror("Error while receiving file chunk");
                fclose(file);
                close(client_socket);
                return;
            }

            fwrite(buf, 1, len, file);
            received_so_far += len;
            total_bytes += len;

            printf("Received %zd bytes in this chunk. Total received for this chunk: %zu/%u bytes.\n", len, received_so_far, chunk_size);
        }

        // printf("Chunk of size %u bytes received successfully.\n", chunk_size);
    }

    // Log successful file transfer
    if (bytes_received < 0) {
        perror("Error while receiving file data");
    } else {
        printf("File '%s' saved successfully.\n", saved_file);
        printf("File size: %zu bytes\n", total_bytes);
    }

    // Clean up resources
    fclose(file);
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <port-number> [bufSize]\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    size_t buf_size = (argc == 3) ? atoi(argv[2]) : DEFAULT_BUF_SIZE;

    char *buf = malloc(buf_size);
    if (!buf) {
        perror("Failed to allocate buffer");
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        free(buf);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        free(buf);
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen on socket");
        close(server_socket);
        free(buf);
        return 1;
    }

    printf("Server listening on port %d with buffer size %zu bytes.\n", port, buf_size);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;
        }

        printf("Connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_client(client_socket, buf, buf_size);
    }

    close(server_socket);
    free(buf);
    return 0;
}

