//
// Created by Daniel on 03.01.2021.
//
#include <logger/logging.h>
#include <sys/socket.h>
#include "clientConnection.h"

int sendFromCache(struct ClientConnection *self, CacheEntry *cache);

int initClientConnection(int clientSocket, ClientConnection **outNewClientConnection) {
    *outNewClientConnection = malloc(sizeof(ClientConnection));
    (*outNewClientConnection)->clientSocket = clientSocket;
    (*outNewClientConnection)->numChunksWritten = 0;
    (*outNewClientConnection)->id = rand() % 9000 + 1000;;

    (*outNewClientConnection)->sendFromCache = &sendFromCache;
}

int closeClientConnection(struct ClientConnection *self) {
    close(self->clientSocket);
    free(self);
    return EXIT_SUCCESS;
}

int sendFromCache(struct ClientConnection *self, CacheEntry *cacheList) {

    return EXIT_SUCCESS;
}
