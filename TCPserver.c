#include <stdio.h>              // For standard input/output functions
#include <stdlib.h>             // For general-purpose functions
#include <string.h>             // For string handling
#include <unistd.h>             // For POSIX functions
#include <arpa/inet.h>          // For internet operations
#include <sys/types.h>          // For socket programming
#include <netinet/in.h>         // For internet address structures
#include <sys/socket.h>         // For socket programming
#include <signal.h>             // For handling signals
#include <errno.h>              // For error codes
#include <ctype.h>              // For character operations

#define DEFAULT_PORT 50918
#define DEFAULT_BUF_SIZE 4096

int serverSocket;

void handle_sigint(int sig) {
    printf("\nCaught signal %d, shutting down the server gracefully...\n", sig);
    if (serverSocket >= 0) {
        close(serverSocket);
    }
    exit(0);
}

void handle_client(int clientSocket, int bufferSize) {
    char fileName[256];
    ssize_t bytesRead = recv(clientSocket, fileName, sizeof(fileName), 0);
    if (bytesRead <= 0) {
        perror("Error receiving file name");
        close(clientSocket);
        return;
    }

    fileName[bytesRead] = '\0'; // Null-terminate file name

    // Handle duplicate file names
    char filePath[300];
    snprintf(filePath, sizeof(filePath), "%s", fileName);

    FILE *file = fopen(filePath, "w");
    if (!file) {
        perror("Error creating file");
        close(clientSocket);
        return;
    }

    // Receive file content
    char *buffer = malloc(bufferSize);
    if (!buffer) {
        perror("Memory allocation error");
        fclose(file);
        close(clientSocket);
        return;
    }

    while ((bytesRead = recv(clientSocket, buffer, bufferSize, 0)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    free(buffer);
    fclose(file);
    close(clientSocket);

    if (bytesRead < 0) {
        perror("Error receiving file content");
    } else {
        printf("File '%s' received successfully\n", filePath);
    }
}

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;
    int bufferSize = (argc > 2) ? atoi(argv[2]) : DEFAULT_BUF_SIZE;

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    signal(SIGINT, handle_sigint);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 5) < 0) {
        perror("Listen failed");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d with buffer size %d\n", port, bufferSize);

    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        handle_client(clientSocket, bufferSize);
    }

    close(serverSocket);
    return EXIT_SUCCESS;
}
