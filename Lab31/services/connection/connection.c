//
// Created by Daniel on 19.12.2020.
//
#include "connection.h"

void dropConnection(int id,
                    const char *reason,
                    int needToCloseServer,
                    Connection *connections,
                    int *connectionsCount,
                    int threadId) {
    printf("Thread: %d, Connection %d, %s\n", threadId, connections[id].id, reason);
    if (connections[id].buffer_size > 0) {
        freeConnectionBuffer(&connections[id]);
    }
    if (needToCloseServer){
        close(connections[id].serverSocket);
    }
    close(connections[id].clientSocket);
    setNotActiveState(id, connections, connectionsCount);
}



void freeConnectionBuffer(Connection *connection) {
    free(connection->buffer);
    connection->buffer = NULL;
    connection->buffer_size = 0;
}


bool isConnectionBufferEmpty(const Connection *connection) {
    return connection->buffer_size == 0;
}

void setNotActiveState(int i, Connection *connections, int *connectionsCount) {

    connections[i] = connections[*connectionsCount - 1];
    connections[*connectionsCount - 1].buffer = NULL;
    connections[*connectionsCount - 1].buffer_size = 0;
    connections[*connectionsCount - 1].cacheIndex = -1;
    connections[*connectionsCount - 1].serverSocket = -1;
    connections[*connectionsCount - 1].clientSocket = -1;
    connections[*connectionsCount - 1].numChunksWritten = 0;
    connections[*connectionsCount - 1].status = NOT_ACTIVE;
    (*connectionsCount)--;
}

void setReadFromServerWriteToClientState(Connection *connection) {
    freeConnectionBuffer(connection);
    connection->status = READ_FROM_SERVER_WRITE_CLIENT;
}

void setReadFromCacheState(Connection *connection, const int cacheIndex) {
    freeConnectionBuffer(connection);
    connection->buffer_size = 0;
    connection->cacheIndex = cacheIndex;
    connection->numChunksWritten = 0;
    connection->status = READ_FROM_CACHE_WRITE_CLIENT;
}

void setWriteToServerState(Connection *connection, const int cacheIndex) {
    connection->status = WRITE_TO_SERVER;
    connection->cacheIndex = cacheIndex;
}

void initNewConnection(Connection *connection, const int newClientSocket) {
    (*connection).clientSocket = newClientSocket;
    (*connection).buffer_size = 0;
    (*connection).buffer = NULL;
    (*connection).cacheIndex = -1;
    (*connection).serverSocket = -1;
    (*connection).status = GETTING_REQUEST_FROM_CLIENT;
    (*connection).id = rand() % 9000 + 1000;
}

int allocateConnectionBufferMemory(Connection *connection, size_t length) {
    connection->buffer_size = length;
    connection->buffer = (char *) calloc(length, sizeof(char));

    if (NULL == connection->buffer) {
        perror("ERROR WHILE MALLOC allocateConnectionBufferMemory");
        return -1;
    }
    return 0;
}

int reallocateConnectionBufferMemory(Connection *connection, size_t additionalLength) {
    connection->buffer_size += additionalLength;
    connection->buffer = (char *) realloc(connection->buffer, additionalLength);
    if (NULL == connection->buffer) {
        perror("ERROR WHILE MALLOC reallocateConnectionBufferMemory");
        return -1;
    }
    return 0;
}