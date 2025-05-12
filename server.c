/******************************************************************************
 * server.c
 *
 * Completed version of a networked ASCII "Battle Game" server in C for Windows.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024
#define GRID_ROWS 5
#define GRID_COLS 5

typedef struct {
    int x, y;
    int hp;
    int active;
} Player;

typedef struct {
    char grid[GRID_ROWS][GRID_COLS];
    Player players[MAX_CLIENTS];
    int clientCount;
} GameState;

GameState g_gameState;
int g_clientSockets[MAX_CLIENTS];
pthread_mutex_t g_stateMutex = PTHREAD_MUTEX_INITIALIZER;

void initGameState() {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            g_gameState.grid[r][c] = '.';
        }
    }
    g_gameState.grid[2][2] = '#';
    g_gameState.grid[1][3] = '#';
    for (int i = 0; i < MAX_CLIENTS; i++) {
        g_gameState.players[i] = (Player){-1, -1, 100, 0};
        g_clientSockets[i] = -1;
    }
    g_gameState.clientCount = 0;
}

void refreshPlayerPositions() {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (g_gameState.grid[r][c] != '#') {
                g_gameState.grid[r][c] = '.';
            }
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_gameState.players[i].active && g_gameState.players[i].hp > 0) {
            int px = g_gameState.players[i].x;
            int py = g_gameState.players[i].y;
            g_gameState.grid[px][py] = 'A' + i;
        }
    }
}

void buildStateString(char *outBuffer) {
    outBuffer[0] = '\0';
    strcat(outBuffer, "STATE\n");
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            char cell[2] = {g_gameState.grid[r][c], '\0'};
            strcat(outBuffer, cell);
        }
        strcat(outBuffer, "\n");
    }
    strcat(outBuffer, "PLAYERS\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_gameState.players[i].active) {
            char line[64];
            snprintf(line, sizeof(line), "%c: (%d,%d) HP: %d\n",
                'A' + i,
                g_gameState.players[i].x,
                g_gameState.players[i].y,
                g_gameState.players[i].hp);
            strcat(outBuffer, line);
        }
    }
}

void broadcastState() {
    char buffer[BUFFER_SIZE];
    buildStateString(buffer);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clientSockets[i] != -1) {
            send(g_clientSockets[i], buffer, strlen(buffer), 0);
        }
    }
}

void handleCommand(int playerIndex, const char *cmd) {
    pthread_mutex_lock(&g_stateMutex);
    Player *p = &g_gameState.players[playerIndex];

    if (strncmp(cmd, "MOVE", 4) == 0) {
        int nx = p->x, ny = p->y;
        if (strstr(cmd, "UP")) nx--;
        else if (strstr(cmd, "DOWN")) nx++;
        else if (strstr(cmd, "LEFT")) ny--;
        else if (strstr(cmd, "RIGHT")) ny++;
        if (nx >= 0 && nx < GRID_ROWS && ny >= 0 && ny < GRID_COLS && g_gameState.grid[nx][ny] != '#') {
            p->x = nx;
            p->y = ny;
        }
    } else if (strncmp(cmd, "ATTACK", 6) == 0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != playerIndex && g_gameState.players[i].active) {
                int dx = abs(g_gameState.players[i].x - p->x);
                int dy = abs(g_gameState.players[i].y - p->y);
                if ((dx + dy) == 1) {
                    g_gameState.players[i].hp -= 10;
                }
            }
        }
    } else if (strncmp(cmd, "QUIT", 4) == 0) {
        p->active = 0;
        closesocket(g_clientSockets[playerIndex]);
        g_clientSockets[playerIndex] = -1;
    }

    refreshPlayerPositions();
    broadcastState();
    pthread_mutex_unlock(&g_stateMutex);
}

void *clientHandler(void *arg) {
    int playerIndex = *(int *)arg;
    free(arg);
    int clientSocket = g_clientSockets[playerIndex];

    pthread_mutex_lock(&g_stateMutex);
    g_gameState.players[playerIndex] = (Player){playerIndex, 0, 100, 1};
    refreshPlayerPositions();
    broadcastState();
    pthread_mutex_unlock(&g_stateMutex);

    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;
        buffer[strcspn(buffer, "\r\n")] = 0;
        handleCommand(playerIndex, buffer);
    }

    pthread_mutex_lock(&g_stateMutex);
    g_gameState.players[playerIndex].active = 0;
    g_clientSockets[playerIndex] = -1;
    refreshPlayerPositions();
    broadcastState();
    pthread_mutex_unlock(&g_stateMutex);

    closesocket(clientSocket);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    int port = atoi(argv[1]);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(serverSocket, MAX_CLIENTS);
    printf("Server started on port %d\n", port);

    initGameState();

    while (1) {
        struct sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) continue;

        pthread_mutex_lock(&g_stateMutex);
        int assigned = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (g_clientSockets[i] == -1) {
                g_clientSockets[i] = clientSocket;
                int *indexPtr = malloc(sizeof(int));
                *indexPtr = i;
                pthread_t tid;
                pthread_create(&tid, NULL, clientHandler, indexPtr);
                pthread_detach(tid);
                assigned = 1;
                break;
            }
        }
        if (!assigned) {
            const char *msg = "Server full\n";
            send(clientSocket, msg, strlen(msg), 0);
            closesocket(clientSocket);
        }
        pthread_mutex_unlock(&g_stateMutex);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
