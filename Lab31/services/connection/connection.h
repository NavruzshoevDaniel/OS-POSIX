//
// Created by Daniel on 19.12.2020.
//

#ifndef LAB31_CONNECTION_H
#define LAB31_CONNECTION_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

enum connectionState {
    GETTING_REQUEST_FROM_CLIENT = 0,
    CONNECTING_TO_SERVER,
    WRITING_REQUEST,
    FORWARDING_REQUEST,
    FORWARDING_RESPONSE,
    RESPONDING_FROM_CACHE,
    NOT_ACTIVE,
    WAITING_DATA_ON_CACHE
};

struct Connection {
    int clientSocket;
    int serverSocket;

    enum connectionState status;
    char *buffer;
    size_t buffer_size;

    int cacheIndex;
    size_t cacheBytesWritten;
    int id;
} typedef Connection;

void dropConnection(int id,
                    const char *reason,
                    Connection *connections,
                    int *connectionsCount,
                    int threadId);

void setNotActiveState(int i, struct Connection *connections, int *connectionsCount);

void freeConnectionBuffer(Connection *connection);


#endif //LAB31_CONNECTION_H
