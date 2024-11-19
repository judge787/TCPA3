# CIS*3210 Assignment 3
## **README**


### **Files in the Project**
1. **`TCPserver.c`**: 
   - The server application that listens for incoming connections, receives files from clients, and stores them on the server's filesystem.
   - Handles duplicate files by appending a numbered suffix (e.g., `(1)`, `(2)`) to the file name.

2. **`TCPclient.c`**: 
   - The client application that connects to the server and sends a specified file.
   - Supports configurable buffer sizes for efficient file transmission.

3. **`spawn_clients.py`**:
   - A Python script to automate testing by simulating multiple clients sending files simultaneously to the server.
   - Measures ping statistics, transfer times, and transfer rates, and prints a summary table of metrics.

4. **`Makefile`**:
   - Simplifies the compilation of the server and client applications.
   - Includes commands for cleaning up compiled binaries.

---
Running the python script 

python3 spawn_clients.py <file-name> <server-IP> <port-number> <client-count> <buffer-size>

Example: python3 spawn_clients.py smallTest.txt 127.0.0.1 50918 20 4096

NOTE: it might take some time for it to complete everything including the summary so once you run it please wait 



Running the Client
Use the client to send a file to the server:
./client <file-name> <server-IP>:<port-number> [buffer-size]

Example:
./client smallTest.txt 127.0.0.1:50918 4096



Running the Server
Start the server with a specified port and buffer size:
./server <port-number> <buffer-size>

Example:
./server 50918 4096

