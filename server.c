/******************************************************************************
 * server.c
 *
 * Template for a networked ASCII "Battle Game" server in C.
 *
 * 1. Create a TCP socket, bind to <PORT>, listen for connections.
 * 2. Accept up to 4 client connections.
 * 3. Manage a global game state: ASCII grid + players + obstacles.
 * 4. On receiving commands (MOVE, ATTACK, QUIT, etc.), update the game state
 *    and broadcast it to all clients.
 *
 * Compile:
 *   gcc server.c -o server -pthread
 *
 * Usage:
 *   ./server <PORT>
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// #include <arpa/inet.h> // Optional if you want to display IP addresses

#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024

/* Grid dimensions */
#define GRID_ROWS 5
#define GRID_COLS 5

/*---------------------------------------------------------------------------*
 * Data Structures
 *---------------------------------------------------------------------------*/

/* Player structure */
typedef struct {
    int x, y;       // current position
    int hp;         // health points
    int active;     // 1 if this player slot is used, 0 otherwise
} Player;

/* Game state: grid + players + count */
typedef struct {
    char grid[GRID_ROWS][GRID_COLS];  // '.' for empty, '#' for obstacle, or 'A'/'B'/'C'/'D'
    Player players[MAX_CLIENTS];
    int clientCount;                  // how many players are connected
} GameState;

/* Global game state */
GameState g_gameState;

/* Store each client's socket; index corresponds to a player ID (0..3) */
int g_clientSockets[MAX_CLIENTS];

/* Mutex to protect shared game state (recommended for thread safety) */
pthread_mutex_t g_stateMutex = PTHREAD_MUTEX_INITIALIZER;


void initGameState() {
    // Fill the grid with '.'
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            g_gameState.grid[r][c] = '.';
        }
    }

    // Example: place some obstacles
    // (Feel free to add more or randomize them)
    g_gameState.grid[2][2] = '#';
    g_gameState.grid[1][3] = '#';

    // Initialize players
    for (int i = 0; i < MAX_CLIENTS; i++) {
        g_gameState.players[i].x = -1;
        g_gameState.players[i].y = -1;
        g_gameState.players[i].hp = 100;
        g_gameState.players[i].active = 0;
        g_clientSockets[i] = -1;
    }

    g_gameState.clientCount = 0;
}

/*---------------------------------------------------------------------------*
 * Refresh the grid with current player positions.
 * We clear old player marks (leaving obstacles) and re-place them according
 * to the players' (x,y).
 *---------------------------------------------------------------------------*/
void refreshPlayerPositions() {
    // Clear all non-obstacle cells
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (g_gameState.grid[r][c] != '#') {
                g_gameState.grid[r][c] = '.';
            }
        }
    }

    // Place each active player's symbol
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_gameState.players[i].active && g_gameState.players[i].hp > 0) {
            int px = g_gameState.players[i].x;
            int py = g_gameState.players[i].y;
            g_gameState.grid[px][py] = 'A' + i; // 'A', 'B', 'C', 'D'
        }
    }
}

/*---------------------------------------------------------------------------*
 * TODO: Build a string that represents the current game state (ASCII grid),
 *       which you can send to all clients.
 *---------------------------------------------------------------------------*/
void buildStateString(char *outBuffer) {
    // e.g., prefix with "STATE\n", then rows of the grid, then player info
    outBuffer[0] = '\0'; // start empty

    strcat(outBuffer, "STATE\n");

    // Copy the grid
    // ...
    // Optionally append player info
}

/*---------------------------------------------------------------------------*
 * Broadcast the current game state to all connected clients
 *---------------------------------------------------------------------------*/
void broadcastState() {
    char buffer[BUFFER_SIZE];
    buildStateString(buffer);

    // TODO: send buffer to each active client via send() or write()
}

/*---------------------------------------------------------------------------*
 * TODO: Handle a client command: MOVE, ATTACK, QUIT, etc.
 *  - parse the string
 *  - update the player's position or HP
 *  - call refreshPlayerPositions() and broadcastState()
 *---------------------------------------------------------------------------*/
void handleCommand(int playerIndex, const char *cmd) {
    // Lock state if needed
    pthread_mutex_lock(&g_stateMutex);

    if (strncmp(cmd, "MOVE", 4) == 0) {
        // Example commands: MOVE UP, MOVE DOWN, MOVE LEFT, MOVE RIGHT
        if (strstr(cmd, "UP")) {
            int nx = g_gameState.players[playerIndex].x - 1;
            int ny = g_gameState.players[playerIndex].y;
            if (nx >= 0 && g_gameState.grid[nx][ny] != '#') {
                g_gameState.players[playerIndex].x = nx;
            }
        } else if (strstr(cmd, "DOWN")) {
            int nx = g_gameState.players[playerIndex].x + 1;
            int ny = g_gameState.players[playerIndex].y;
            if (nx < GRID_ROWS && g_gameState.grid[nx][ny] != '#') {
                g_gameState.players[playerIndex].x = nx;
            }
        }
        // else if (strstr(cmd, "LEFT")) { ... }
        // else if (strstr(cmd, "RIGHT")) { ... }
    }
    // else if (strncmp(cmd, "ATTACK", 6) == 0) { ... }
    // else if (strncmp(cmd, "QUIT", 4) == 0) { ... }

    // Refresh positions and broadcast
    refreshPlayerPositions();
    broadcastState();

    // Unlock
    pthread_mutex_unlock(&g_stateMutex);
}

/*---------------------------------------------------------------------------*
 * Thread function: handle communication with one client
 *---------------------------------------------------------------------------*/
void *clientHandler(void *arg) {
    int playerIndex = *(int *)arg;
    free(arg);

    int clientSocket = g_clientSockets[playerIndex];

    // Example: set player's initial position
    pthread_mutex_lock(&g_stateMutex);
    g_gameState.players[playerIndex].x = playerIndex; // naive approach
    g_gameState.players[playerIndex].y = 0;
    g_gameState.players[playerIndex].active = 1;
    refreshPlayerPositions();
    broadcastState();
    pthread_mutex_unlock(&g_stateMutex);

    char buffer[BUFFER_SIZE];

    // Main recv loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        // TODO: recv from clientSocket
        // ...

        // Strip trailing newline (if any)
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        // Handle the command
        handleCommand(playerIndex, buffer);

        // If the player used QUIT in handleCommand, we can also break here:
        pthread_mutex_lock(&g_stateMutex);
        if (g_gameState.players[playerIndex].active == 0) {
            pthread_mutex_unlock(&g_stateMutex);
            break;
        }
        pthread_mutex_unlock(&g_stateMutex);
    }

    // Cleanup on disconnect
    printf("Player %c disconnected.\n", 'A' + playerIndex);
    close(clientSocket);

    // Mark inactive
    pthread_mutex_lock(&g_stateMutex);
    g_clientSockets[playerIndex] = -1;
    g_gameState.players[playerIndex].active = 0;
    refreshPlayerPositions();
    broadcastState();
    pthread_mutex_unlock(&g_stateMutex);

    return NULL;
}

/*---------------------------------------------------------------------------*
 * main: set up server socket, accept clients, spawn threads
 *---------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    // 1. Initialize game state
    initGameState();

    // 2. Create server socket
    // int serverSock = socket(...);
    // TODO

    // 3. Bind, listen
    // struct sockaddr_in serverAddr;
    // ...
    // bind(...)
    // listen(...)

    printf("Server listening on port %d...\n", port);

    // 4. Accept loop
    while (1) {
        // int newSock = accept(...);
        // if (newSock < 0) { continue; }

        // If we have capacity, find a free index in g_clientSockets
        // create a thread: pthread_t tid;
        // int *arg = malloc(sizeof(int));
        // *arg = freeIndex;
        // pthread_create(&tid, NULL, clientHandler, arg);
        // pthread_detach(tid);
    }

    // close(serverSock);
    return 0;
}
