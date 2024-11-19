import sys
import subprocess
from threading import Thread
import time
import platform
import os


def run_client(file, server_ip, port, buffer_size, client_id, transfer_rates, file_size):
    """
    Function to execute the client process using subprocess and measure transfer time.
    """
    start_time = time.time()  # Record start time
    try:
        command = ["./client", file, f"{server_ip}:{port}", str(buffer_size)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        end_time = time.time()  # Record end time

        # Calculate duration
        duration = end_time - start_time
        transfer_rate = file_size / duration  # Bytes/second
        transfer_rate_mb = transfer_rate / (1024 * 1024)  # Convert to MB/s
        transfer_rates.append(transfer_rate_mb)

        return duration
    except Exception:
        return None


def measure_ping(server_ip):
    """
    Measure ping times to the server and return statistics.
    """
    # Set up the ping command based on the platform
    ping_count = 20
    if platform.system().lower() == "windows":
        command = ["ping", "-n", str(ping_count), server_ip]
    else:
        command = ["ping", "-c", str(ping_count), server_ip]

    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        # Extract statistics (works for most Unix-based systems)
        ping_stats = {"min": None, "avg": None, "max": None, "stddev": None, "packet_loss": None}
        if platform.system().lower() != "windows":
            lines = result.stdout.splitlines()

            # Extract RTT statistics
            stats_line = next((line for line in lines if "rtt min/avg/max/mdev" in line), None)
            if stats_line:
                stats = stats_line.split("=")[1].strip()
                min_, avg, max_, stddev = stats.split("/")
                ping_stats["min"] = float(min_.strip(" ms"))
                ping_stats["avg"] = float(avg.strip(" ms"))
                ping_stats["max"] = float(max_.strip(" ms"))
                ping_stats["stddev"] = float(stddev.strip(" ms"))

            # Extract packet statistics
            packet_line = next((line for line in lines if "packets transmitted" in line), None)
            if packet_line:
                parts = packet_line.split(",")
                transmitted = int(parts[0].split()[0])
                received = int(parts[1].split()[0])
                packet_loss = float(parts[2].split("%")[0])
                ping_stats["packet_loss"] = packet_loss

        return ping_stats
    except Exception:
        return None


def print_summary_table(ping_stats, transfer_metrics):
    """
    Print a summary table of metrics using plain text formatting.
    """
    print("\n=== Summary Table ===")
    print(f"{'Metric':<25}{'Value':<15}")
    print("-" * 40)

    if ping_stats:
        print(f"{'Ping Min (ms)':<25}{ping_stats['min'] or 'N/A':<15}")
        print(f"{'Ping Avg (ms)':<25}{ping_stats['avg'] or 'N/A':<15}")
        print(f"{'Ping Max (ms)':<25}{ping_stats['max'] or 'N/A':<15}")
        print(f"{'Ping Stddev (ms)':<25}{ping_stats['stddev'] or 'N/A':<15}")
        print(f"{'Packet Loss (%)':<25}{ping_stats['packet_loss'] or 'N/A':<15}")
    else:
        print(f"{'Ping Metrics':<25}{'N/A':<15}")

    print(f"{'Avg Transfer Time (s)':<25}{transfer_metrics['avg_time']:<15.2f}")
    print(f"{'Min Transfer Time (s)':<25}{transfer_metrics['min_time']:<15.2f}")
    print(f"{'Max Transfer Time (s)':<25}{transfer_metrics['max_time']:<15.2f}")
    print(f"{'Avg Transfer Rate (MB/s)':<25}{transfer_metrics['avg_rate']:<15.2f}")
    print(f"{'Min Transfer Rate (MB/s)':<25}{transfer_metrics['min_rate']:<15.2f}")
    print(f"{'Max Transfer Rate (MB/s)':<25}{transfer_metrics['max_rate']:<15.2f}")


def main():
    # Validate arguments
    if len(sys.argv) < 6:
        print("Usage: python3 spawn_clients.py <file> <server_ip> <port> <client_count> <buffer_size>")
        sys.exit(1)

    file = sys.argv[1]
    server_ip = sys.argv[2]
    port = int(sys.argv[3])
    client_count = int(sys.argv[4])
    buffer_size = int(sys.argv[5])

    # Get file size for transfer rate calculation
    try:
        file_size = os.path.getsize(file)  # Size in bytes
    except Exception:
        print("Error reading file size. Make sure the file exists.")
        sys.exit(1)

    # Measure ping times
    ping_stats = measure_ping(server_ip)

    # Collect transfer times and rates
    transfer_times = []
    transfer_rates = []

    threads = []
    for i in range(client_count):
        thread = Thread(
            target=lambda: transfer_times.append(
                run_client(file, server_ip, port, buffer_size, i + 1, transfer_rates, file_size)
            )
        )
        thread.start()
        threads.append(thread)

    # Wait for all threads to finish
    for thread in threads:
        thread.join()

    # Calculate and display metrics
    if transfer_times:
        valid_times = [t for t in transfer_times if t is not None]
        valid_rates = [r for r in transfer_rates if r is not None]

        if valid_times:
            transfer_metrics = {
                "avg_time": sum(valid_times) / len(valid_times),
                "min_time": min(valid_times),
                "max_time": max(valid_times),
                "avg_rate": sum(valid_rates) / len(valid_rates),
                "min_rate": min(valid_rates),
                "max_rate": max(valid_rates),
            }
            print_summary_table(ping_stats, transfer_metrics)
        else:
            print("No valid transfer times recorded.")
    else:
        print("No transfers were completed.")


if __name__ == "__main__":
    main()
