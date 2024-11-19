#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_BUF_SIZE 4096 // Default buffer size for reading file data

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <fileName> <IP-address>:<port-number> [bufSize]\n", argv[0]);
        return 1;
    }

    char *fileName = argv[1];
    char *ipPort = argv[2];
    int bufSize = (argc == 4) ? atoi(argv[3]) : DEFAULT_BUF_SIZE;

    char *colon = strchr(ipPort, ':');
    if (!colon) {
        printf("Invalid IP:Port format\n");
        return 1;
    }
    *colon = '\0';
    char *ipAddress = ipPort;
    int port = atoi(colon + 1);

    FILE *file = fopen(fileName, "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error creating socket");
        fclose(file);
        return 1;
    }

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

   printf("Connecting to server at %s:%d...\n", ipAddress, port);
   if (connect (clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        close(clientSocket);
        fclose(file);
        return 1;
    }

    send(clientSocket, fileName, strlen(fileName) + 1, 0);

    char ack[16];
    if (recv(clientSocket, ack, sizeof(ack), 0) <= 0 || strcmp(ack, "READY") != 0) {
        perror("Server not ready for file transfer");
        fclose(file);
        close(clientSocket);
        return 1;
    }
    printf("Server acknowledged file name. Starting transfer.\n");

    char *buffer = malloc(bufSize);
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, bufSize, file)) > 0) {
        uint32_t chunkSize = htonl(bytesRead);
        send(clientSocket, &chunkSize, sizeof(chunkSize), 0);
        send(clientSocket, buffer, bytesRead, 0);

        printf("Sent chunk of size %zu bytes.\n", bytesRead);
    }

    uint32_t endOfFile = 0;
    send(clientSocket, &endOfFile, sizeof(endOfFile), 0);

    free(buffer);
    fclose(file);
    close(clientSocket);
    printf("File '%s' sent successfully\n", fileName);
    return 0;
}
