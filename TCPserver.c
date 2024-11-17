#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_BUF_SIZE 4096 // Default buffer size

void handle_client(int client_socket, char *buf, int buf_size) {
    char file_name[256];
    int received = recv(client_socket, file_name, sizeof(file_name), 0); // Receive file name
    if (received <= 0) {
        perror("Failed to receive file name");
        close(client_socket);
        return;
    }

    // Handle duplicate file names
    char saved_file[512];
    snprintf(saved_file, sizeof(saved_file), "./%s", file_name);
    int suffix = 1;
    while (access(saved_file, F_OK) == 0) { // Ensure no overwriting
        snprintf(saved_file, sizeof(saved_file), "./%s(%d)", file_name, suffix++);
    }

    FILE *file = fopen(saved_file, "wb"); // Open file to write
    if (!file) {
        perror("Failed to create file");
        close(client_socket);
        return;
    }

    ssize_t bytes;
    size_t total_bytes = 0; // Total size of the file
    while ((bytes = recv(client_socket, buf, buf_size, 0)) > 0) { // Receive chunks
        fwrite(buf, 1, bytes, file);
        total_bytes += bytes;
    }

    if (bytes < 0) {
        perror("Error while receiving file data");
    } else {
        printf("File '%s' saved successfully.\n", saved_file);
        printf("File size: %zu bytes\n", total_bytes); // Display total file size
    }

    fclose(file);
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <port-number> [bufSize]\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]); // Port number from arguments
    int buf_size = (argc == 3) ? atoi(argv[2]) : DEFAULT_BUF_SIZE; // Buffer size from arguments
    char *buf = malloc(buf_size); // Allocate buffer
    if (!buf) {
        perror("Failed to allocate buffer");
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0); // Create socket
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

    printf("Server listening on port %d with buffer size %d bytes.\n", port, buf_size);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;
        }

        printf("Connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); // Log client IP and port
        printf("Buffer size: %d bytes\n", buf_size); // Log buffer size

        handle_client(client_socket, buf, buf_size); // Handle the client
    }

    close(server_socket);
    free(buf);
    return 0;
}
