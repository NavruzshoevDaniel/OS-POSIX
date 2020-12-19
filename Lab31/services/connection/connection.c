//
// Created by Daniel on 19.12.2020.
//
#include "connection.h"

void dropConnection(int id,
               const char *reason,
               Connection *connections,
               int *connectionsCount,
               int threadId) {
    printf("Thread: %d, Connection %d, %s\n", threadId, connections[id].id, reason);
    if (connections[id].buffer_size > 0) {
        freeConnectionBuffer(&connections[id]);
    }
    close(connections[id].clientSocket);

    setNotActiveState(id, connections, connectionsCount);
}

void freeConnectionBuffer(Connection *connection) {
    free(connection->buffer);
    connection->buffer = NULL;
    connection->buffer_size = 0;
}

void setNotActiveState(int i, struct Connection *connections, int *connectionsCount) {
    connections[i] = connections[*connectionsCount - 1];
    connections[*connectionsCount - 1].buffer = NULL;
    connections[*connectionsCount - 1].buffer_size = 0;
    connections[*connectionsCount - 1].cacheIndex = -1;
    connections[*connectionsCount - 1].serverSocket = -1;
    connections[*connectionsCount - 1].clientSocket = -1;
    connections[*connectionsCount - 1].cacheBytesWritten = 0;
    connections[*connectionsCount - 1].status = NOT_ACTIVE;
    (*connectionsCount)--;
}