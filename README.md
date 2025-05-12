# Networked ASCII "Battle Game"

## Description:
Battle Game is a text-based networked game which utilizes TCP sockets for real-time communication. Players will have their HP and grid position maintained, and will be allowed to move accross the board, attack other players, or senc chat messages. Up to 4 players can play at once.

## Quickstart guide:
1. **Server**
   
To run the server, enter the following into terminal, where <PORT> is a chosen port.
```shell
python server.py <PORT>
```
Example with port 12345:
```shell
python server.py 12345
```

2. **Client**
   
For each client, run the following in a seperate terminal, where <PORT> is the port from above and <SERVER> is the server IP (local host):
```shell
python client.py <SERVER> <PORT>
```
Example with server IP and port 12345:
```shell
python client.py 127.0.0.1 12345
```

## Gameplay:
- Obstacles are represented by "#" and can not be moved accross
- Open spaces are represented by "."
- Players are represented by A,B,C,D
- Players can move up, down, left, or right if within the board and no obstacles are in the way
- Players can attack another player if they are adjacent to them on the board
- If a player is attacked, they will lose HP
- Players can send chat messages

The following commands can be used in the game:
- **MOVE UP**: Move player one space up
- **MOVE DOWN**: Move player one space down
- **MOVE LEFT**: Move player one space left
- **MOVE RIGHT**: Move player one space right
- **ATTACK**: Attack adjacent player
- **CHAT <MSG>**: Send chat message
- **QUIT**: Exit game

## Protocol overview:
-	Client connects to server’s IP address and specified port, creating a player
-	Client reads user commands and sends them to server (e.g. MOVE, ATTACK, QUIT, CHAT)
-	Server processes command, then updates the state
-	The client receives the update, then displays them in ASCII grid format
-	Client disconnects when user enters QUIT


---

(Previous README content):
## Overview

This assignment requires you to implement a **client-server** ASCII-based game using **TCP sockets**.  
You will:
1. **Design an application-level protocol** (how clients and server exchange messages).
2. **Implement the socket code** (on both the server and client side).
3. **Manage concurrency** on the server (e.g., using threads or I/O multiplexing).
4. **Implement game logic** for a simple “battle” scenario with movement, attacks, and obstacles.

You may complete this assignment in **C** or **Python** (both templates are provided).

---

## Requirements

1. **Server**
    - Listens on a TCP socket.
    - Accepts up to 4 clients.
    - Maintains a **GameState** (a 2D grid).
    - Supports commands like **MOVE**, **ATTACK**, **QUIT**, etc.
    - After each valid command, it **broadcasts** the updated game state to all players.

2. **Client**
    - Connects to the server via TCP.
    - Sends user-typed commands.
    - Continuously **receives** and displays updates of the ASCII grid (plus any extra info).

3. **Protocol**
    - Must be documented (in code comments or a separate file/section).
    - Could be plain text like `"MOVE UP"`, `"ATTACK"`, `"QUIT"`, etc.
    - Server can respond with messages like `"STATE\n"` (followed by the ASCII grid) or `"ERROR\n"` if needed.

4. **Game Logic**
    - 2D grid (e.g., 5×5).
    - **Obstacles** (`#`) block movement.
    - **Players** are labeled `'A', 'B', 'C', 'D'`.
    - **Attacks**: Deduct HP from adjacent players or define your own rules.
    - **Quit**: On `QUIT`, the client disconnects, and the server updates the state accordingly.

5. **Extra Features** (Optional)
    - Turn-based mechanics.
    - More advanced attacking.
    - Security checks (e.g., invalid commands).

---

## Instructions (C Version)

1. Open `server.c` and `client.c`.
2. Complete all **`TODO`** sections.
3. **Compile**:
   ```bash
   # Server
   gcc server.c -o server -pthread

   # Client
   gcc client.c -o client -pthread
   ```
   
    Or use `clang`:

    ```bash
    clang server.c -o server -pthread
    clang client.c -o client -pthread
    ```

4. **Run**:
    ```bash
    # Terminal 1: start the server on port 12345
    ./server 12345

    # Terminal 2..N: start clients
    ./client 127.0.0.1 12345
    ```
   
---

## Instructions (Python Version)

1. Open `server.py` and `client.py`.
2. Complete all **TODO** sections.
3. **Run**:
```shell
# Server
python server.py 12345

# Client
python client.py 127.0.0.1 12345
```
Where `12345` is your chosen port, and `127.0.0.1` is the server IP (local host).

**Have fun building your networked ASCII Battle Game!**
