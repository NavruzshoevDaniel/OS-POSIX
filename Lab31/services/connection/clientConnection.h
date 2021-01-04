//
// Created by Daniel on 03.01.2021.
//

#ifndef LAB31_CLIENTCONNECTION_H
#define LAB31_CLIENTCONNECTION_H

#include <cache/cache.h>
#include <unistd.h>

struct ClientConnection {
    int clientSocket;
    int id;
    int numChunksWritten;

    int (*sendFromCache)(struct ClientConnection *self, CacheEntry *cache);

} typedef ClientConnection;

int initClientConnection(int clientSocket, ClientConnection **outNewClientConnection);

int closeClientConnection(struct ClientConnection *self);

#endif //LAB31_CLIENTCONNECTION_H
