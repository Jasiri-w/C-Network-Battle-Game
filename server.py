#!/usr/bin/env python3
"""
server.py
Skeleton for a networked ASCII "Battle Game" server in Python.

1. Create a TCP socket and bind to <PORT>.
2. Listen and accept up to 4 client connections.
3. Maintain a global game state (grid + players + obstacles).
4. On receiving commands (MOVE, ATTACK, QUIT, etc.), update the game state
   and broadcast the updated state to all connected clients.

Usage:
   python server.py <PORT>
"""

import sys
import socket
import threading

MAX_CLIENTS = 4
BUFFER_SIZE = 1024
GRID_ROWS = 5
GRID_COLS = 5

###############################################################################
# Data Structures
###############################################################################

# Each player can be stored as a dict, for instance:
# {
#    'x': int,
#    'y': int,
#    'hp': int,
#    'active': bool
# }

# The global game state might be stored in a dictionary:
# {
#   'grid': [ [char, ...], ... ],        # 2D list of chars
#   'players': [ playerDict, playerDict, ...],
#   'clientCount': int
# }

g_gameState = {}
g_clientSockets = [None] * MAX_CLIENTS  # track client connections
g_stateLock = threading.Lock()          # lock for the game state

###############################################################################
# Initialize the game state
###############################################################################
def initGameState():
    global g_gameState
    # Create a 2D grid filled with '.'
    grid = []
    for r in range(GRID_ROWS):
        row = ['.'] * GRID_COLS
        grid.append(row)

    # Example: place a couple of obstacles '#'
    # (Feel free to add more or randomize them.)
    grid[2][2] = '#'
    grid[1][3] = '#'

    # Initialize players
    players = []
    for i in range(MAX_CLIENTS):
        p = {
            'x': -1,
            'y': -1,
            'hp': 100,
            'active': False
        }
        players.append(p)

    g_gameState = {
        'grid': grid,
        'players': players,
        'clientCount': 0,
        'chat_history' : [],
        'chat_sent' : False
    }

###############################################################################
# Refresh the grid with current player positions.
# We clear old player marks (leaving obstacles) and re-place them according
# to the players' (x,y).
###############################################################################
def refreshPlayerPositions():
    """Clear old positions (leaving obstacles) and place each active player."""
    grid = g_gameState['grid']
    players = g_gameState['players']

    # Clear non-obstacle cells
    for r in range(GRID_ROWS):
        for c in range(GRID_COLS):
            if grid[r][c] != '#':
                grid[r][c] = '.'

    # Place each active player
    for i, player in enumerate(players):
        if player['active'] and player['hp'] > 0:
            px = player['x']
            py = player['y']
            grid[px][py] = chr(ord('A') + i)  # 'A', 'B', 'C', 'D'

###############################################################################
# TODO: Build a string that represents the current game state (ASCII grid), 
# which you can send to all clients.
###############################################################################
def buildStateString():
    buffer = ["STATE\n"]
    grid = g_gameState['grid']

    # Add grid rows
    for row in grid:
        buffer.append(''.join(row) + '\n')

    buffer.append("PLAYERS\n")
    for i, player in enumerate(g_gameState['players']):
        if player['active']:
            name = chr(ord('A') + i)
            pos = f"({player['x']},{player['y']})"
            hp = player['hp']
            buffer.append(f"Player {name}: HP={hp} Pos={pos}\n")

    if g_gameState['chat_sent']:
        buffer.append("CHAT\n")
        buffer.append(f"{g_gameState['chat_history'][-1]}\n")
        print(f"Player {g_gameState['chat_history'][-1]}")
        g_gameState['chat_sent'] = False

    return ''.join(buffer)


###############################################################################
# Broadcast the current game state to all connected clients
###############################################################################
def broadcastState():
    stateStr = buildStateString().encode('utf-8')
    for sock in g_clientSockets:
        if sock:
            try:
                sock.sendall(stateStr)
            except:
                continue  # Ignore failures silently


###############################################################################
# TODO: Handle a client command: MOVE, ATTACK, QUIT, etc.
#  - parse the string
#  - update the player's position or HP
#  - call refreshPlayerPositions() and broadcastState()
###############################################################################
def handleCommand(playerIndex, cmd):
    with g_stateLock:
        players = g_gameState['players']
        player = players[playerIndex]
        x, y = player['x'], player['y']

        if cmd.startswith("MOVE"):
            dx, dy = 0, 0
            if "UP" in cmd: dx = -1
            elif "DOWN" in cmd: dx = 1
            elif "LEFT" in cmd: dy = -1
            elif "RIGHT" in cmd: dy = 1

            nx, ny = x + dx, y + dy
            if 0 <= nx < GRID_ROWS and 0 <= ny < GRID_COLS:
                if g_gameState['grid'][nx][ny] != '#':
                    player['x'], player['y'] = nx, ny

        elif cmd.startswith("ATTACK"):
            for i, other in enumerate(players):
                if i != playerIndex and other['active'] and other['hp'] > 0:
                    ox, oy = other['x'], other['y']
                    if abs(x - ox) + abs(y - oy) == 1:  # Adjacent
                        other['hp'] -= 20
                        if other['hp'] <= 0:
                            other['active'] = False

        elif cmd.startswith("QUIT"):
            player['active'] = False
            if g_clientSockets[playerIndex]:
                g_clientSockets[playerIndex].close()
                g_clientSockets[playerIndex] = None
            g_gameState['clientCount'] -= 1
        
        elif cmd.startswith("CHAT"):
            msg = f"{chr(ord('A') + playerIndex)}: {cmd[4:]}"
            g_gameState['chat_history'].append(msg)
            g_gameState['chat_sent'] = True
        
        

        refreshPlayerPositions()
        broadcastState()


###############################################################################
# Thread function: handle communication with one client
###############################################################################
def clientHandler(playerIndex):
    sock = g_clientSockets[playerIndex]

    # Initialize player
    with g_stateLock:
        g_gameState['players'][playerIndex]['x'] = playerIndex  # naive example
        g_gameState['players'][playerIndex]['y'] = 0
        g_gameState['players'][playerIndex]['hp'] = 100
        g_gameState['players'][playerIndex]['active'] = True
        refreshPlayerPositions()
        broadcastState()

    while True:
        try:
            # TODO: receive from client socket
            data = sock.recv(1024).decode('utf-8').strip()
            if not data:
                break  

            handleCommand(playerIndex, data)
        except:
            break  # On error, break out

    # Cleanup on disconnect
    print(f"Player {chr(ord('A') + playerIndex)} disconnected.")
    sock.close()

    with g_stateLock:
        g_clientSockets[playerIndex] = None
        g_gameState['players'][playerIndex]['active'] = False
        refreshPlayerPositions()
        broadcastState()

###############################################################################
# main: set up server socket, accept clients, spawn threads
###############################################################################
def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <PORT>")
        sys.exit(1)

    port = int(sys.argv[1])
    initGameState()

    serverSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serverSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serverSock.bind(('', port))
    serverSock.listen(5)
    serverSock.settimeout(1.0) 

    print(f"Server listening on port {port}...")

    while True:
        try:
            clientSock, addr = serverSock.accept()
            print(f"Accepted new client from {addr}")

            with g_stateLock:
                if g_gameState['clientCount'] >= MAX_CLIENTS:
                    print("Server full. Rejecting client.")
                    clientSock.sendall(b"Server full. Try again later.\n")
                    clientSock.close()
                    continue

                # Find a free slot
                slot = None
                for i in range(MAX_CLIENTS):
                    if g_clientSockets[i] is None:
                        slot = i
                        break

                if slot is None:
                    print("No free slot found, though client count is under max.")
                    clientSock.sendall(b"Server error. Try again later.\n")
                    clientSock.close()
                    continue

                g_clientSockets[slot] = clientSock
                g_gameState['clientCount'] += 1

            thread = threading.Thread(target=clientHandler, args=(slot,), daemon=True)
            thread.start()


        except socket.timeout:
                continue
        except KeyboardInterrupt:
            print("\nServer shutting down.")
            break
        except Exception as e:
            print(f"Error accepting client: {e}")
            continue

    serverSock.close()


if __name__ == "__main__":
    main()
