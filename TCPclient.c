
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <fileName> <IP-address>:<port-number> [bufSize]\n", argv[0]);
        return 1;
    }

    char *fileName = argv[1];
    char *ipPort = argv[2];
    int bufSize = (argc == 4) ? atoi(argv[3]) : DEFAULT_BUF_SIZE;

    // Split IP and port
    char *colon = strchr(ipPort, ':');
    if (!colon) {
        printf("Invalid IP:Port format\n");
        return 1;
    }
    *colon = '\0';
    char *ipAddress = ipPort;
    int port = atoi(colon + 1);

    // Open the file
    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // Create the socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error creating socket");
        fclose(file);
        return 1;
    }

    // Server address setup
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(clientSocket);
        fclose(file);
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        close(clientSocket);
        fclose(file);
        return 1;
    }

    // Send the file name
    send(clientSocket, fileName, strlen(fileName) + 1, 0);

    // Send the file contents
    char *buffer = malloc(bufSize);
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, bufSize, file)) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
    }

    // Clean up
    free(buffer);
    fclose(file);
    close(clientSocket);
    printf("File '%s' sent successfully\n", fileName);
    return 0;
}
