import threading
import subprocess
import sys

def run_client(params):
    subprocess.run(["./bin/HTTPclient"] + params)  # Pass all client parameters

def main():
    if len(sys.argv) < 5 or sys.argv[1] != "-n" or sys.argv[3] != "HTTPclient":
        print("Usage: stress -n <num_threads> HTTPclient <client_params>")
        return
    
    try:
        num_threads = int(sys.argv[2])
        if num_threads <= 0:
            raise ValueError
    except ValueError:
        print("Error: <num_threads> must be a positive integer.")
        return

    client_params = sys.argv[4:]  # Collect all parameters after "HTTPclient"
    
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=run_client, args=(client_params,))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()

if __name__ == "__main__":
    main()