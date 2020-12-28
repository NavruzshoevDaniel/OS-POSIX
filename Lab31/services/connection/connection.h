//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_CONNECTION_H
#define LAB31_CONNECTION_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

enum connectionState {
    GETTING_REQUEST_FROM_CLIENT = 0,
    WRITE_TO_SERVER,
    READ_FROM_SERVER_WRITE_CLIENT,
    READ_FROM_CACHE_WRITE_CLIENT,
    NOT_ACTIVE
};

struct Connection {
    int clientSocket;
    int serverSocket;

    enum connectionState status;
    char *buffer;
    size_t buffer_size;

    int cacheIndex;
    size_t numChunksWritten;
    int id;
} typedef Connection;

void dropConnection(int id,
                    const char *reason,
                    int needToCloseServer,
                    Connection *connections,
                    int *connectionsCount,
                    int threadId);

void setNotActiveState(int i, struct Connection *connections, int *connectionsCount);

void setReadFromCacheState(Connection *connection, int cacheIndex);

void setWriteToServerState(Connection *connection, int cacheIndex);

void setReadFromServerWriteToClientState(Connection *connection);

bool isConnectionBufferEmpty(const Connection *connection);

void initNewConnection(Connection *connection, int newClientSocket);

int allocateConnectionBufferMemory(Connection *connection, size_t length);

int reallocateConnectionBufferMemory(Connection *connection, size_t additionalLength);

void freeConnectionBuffer(Connection *connection);



#endif //LAB31_CONNECTION_H
