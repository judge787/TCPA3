import sys
import subprocess
from threading import Thread

def run_client(file, server_ip, port, buffer_size):
    """
    Function to execute the client process using subprocess.
    """
    try:
        command = ["./client", file, f"{server_ip}:{port}", str(buffer_size)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print(f"Client finished with output:\n{result.stdout}")
        if result.stderr:
            print(f"Client error:\n{result.stderr}")
    except Exception as e:
        print(f"Error running client: {e}")

def main():
    if len(sys.argv) < 6:
        print("Usage: python3 spawn_clients.py <file> <server_ip> <port> <client_count> <buffer_size>")
        sys.exit(1)

    file = sys.argv[1]
    server_ip = sys.argv[2]
    port = int(sys.argv[3])
    client_count = int(sys.argv[4])
    buffer_size = int(sys.argv[5])

    threads = []
    for i in range(client_count):
        print(f"Starting client {i + 1}")
        thread = Thread(target=run_client, args=(file, server_ip, port, buffer_size))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()

    print("All clients finished.")

if __name__ == "__main__":
    main()
