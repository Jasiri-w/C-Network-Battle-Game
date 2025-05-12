#!/usr/bin/env python3
"""
client.py
Skeleton for a networked ASCII "Battle Game" client in Python.

1. Connect to the server via TCP.
2. Continuously read user input (MOVE, ATTACK, QUIT).
3. Send commands to the server.
4. Spawn a thread to receive and display the updated game state from the server.

Usage:
   python client.py <SERVER_IP> <PORT>
"""

import sys
import socket
import threading

BUFFER_SIZE = 1024
g_serverSocket = None  # shared by main thread and receiver thread

###############################################################################
# TODO: continuously receive updates (ASCII grid) from the server
###############################################################################
def receiverThread():
    global g_serverSocket
    while True:
        try:
            data = g_serverSocket.recv(BUFFER_SIZE).decode('utf-8')
            if not data:
                break 

            print("\r" + data, end='')
            print("Enter command (MOVE/ATTACK/QUIT/CHAT <MSG>): ", end='', flush=True)
        except:
            break  


    g_serverSocket.close()
    sys.exit(0)

###############################################################################
# main: connect to server, spawn receiver thread, send commands in a loop
###############################################################################
def main():
    global g_serverSocket

    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <SERVER_IP> <PORT>")
        sys.exit(1)

    serverIP = sys.argv[1]
    port = int(sys.argv[2])

    # TODO: create socket & connect to server
    # Example:
    g_serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    g_serverSocket.connect((serverIP, port))
    print(f"Connected to server {serverIP}:{port}")

    # Spawn receiver thread
    t = threading.Thread(target=receiverThread, args=())
    t.daemon = True
    t.start()

    # Main loop: read commands, send to server
    while True:
        try:
            cmd = input("")
        except EOFError:
            # e.g., Ctrl+D
            print("Exiting client.")
            break

        if not cmd:  # empty line
            continue

        # TODO: send command to server
        g_serverSocket.sendall(cmd.encode('utf-8'))

        # If QUIT => break
        if cmd.upper().startswith("QUIT"):
            break

    # cleanup
    if g_serverSocket:
        g_serverSocket.close()
    sys.exit(0)

if __name__ == "__main__":
    main()
