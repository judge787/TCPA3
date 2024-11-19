import sys
import subprocess
from threading import Thread
import socket
import os
import shutil


def run_client(file, server_ip, port, buffer_size, client_id):
    """
    Function to execute the client process using subprocess.
    """
    try:
        command = ["./client", file, f"{server_ip}:{port}", str(buffer_size)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        print(f"Client {client_id} finished with output:\n{result.stdout}")
        if result.stderr:
            print(f"Client {client_id} error:\n{result.stderr}")
    except Exception as e:
        print(f"Error running client {client_id}: {e}")


def check_server(host, port):
    """
    Check if the server is reachable.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((host, port))
            return True
        except Exception:
            return False


def create_unique_files(original_file, client_count):
    """
    Create unique copies of the input file for each client.
    """
    unique_files = []
    for i in range(client_count):
        unique_file = f"{original_file}_client{i + 1}"
        shutil.copyfile(original_file, unique_file)
        unique_files.append(unique_file)
    return unique_files


def cleanup_files(files):
    """
    Clean up temporary files created for testing.
    """
    for file in files:
        if os.path.exists(file):
            os.remove(file)


def main():
    if len(sys.argv) < 6:
        print("Usage: python3 spawn_clients.py <file> <server_ip> <port> <client_count> <buffer_size>")
        sys.exit(1)

    original_file = sys.argv[1]
    server_ip = sys.argv[2]
    port = int(sys.argv[3])
    client_count = int(sys.argv[4])
    buffer_size = int(sys.argv[5])

    # Check if the server is reachable
    if not check_server(server_ip, port):
        print(f"Server at {server_ip}:{port} is not reachable. Exiting.")
        sys.exit(1)

    # Create unique files for each client
    print("Preparing unique files for clients...")
    unique_files = create_unique_files(original_file, client_count)

    threads = []
    for i in range(client_count):
        print(f"Starting client {i + 1}")
        thread = Thread(target=run_client, args=(unique_files[i], server_ip, port, buffer_size, i + 1))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()

    print("All clients finished. Cleaning up temporary files...")
    cleanup_files(unique_files)


if __name__ == "__main__":
    main()
