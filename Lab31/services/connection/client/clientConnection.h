//
// Created by Daniel on 03.01.2021.
//

#ifndef LAB31_CLIENTCONNECTION_H
#define LAB31_CLIENTCONNECTION_H

#include <cache/cache.h>
#include <unistd.h>
#include <poll.h>
#include <connection/server/serverConnectionList.h>

#define DEAD_CLIENT_EXCEPTION -2
#define RECV_CLIENT_EXCEPTION -3
#define ALLOCATING_BUFFER_MEMORY_EXCEPTION -4
#define NOT_GET_EXCEPTION -5
#define URL_EXCEPTION -6
#define RESOLVING_SOCKET_FROM_URL_EXCEPTION -7

#define SEND_TO_CLIENT_EXCEPTION -22
#define WRITER_CACHE_INVALID_EXCEPTION -33
#define CACHE_INVALID_EXCEPTION -44
#define SUCCESS_WITH_END 11

enum ClientState {
    WAITING_REQUEST,
    SENDING_FROM_CACHE
} typedef ClientState;

struct ClientConnection {
    int clientSocket;
    int id;
    int numChunksWritten;
    int cacheIndex;
    ClientState state;
    struct pollfd *fd;
/**
 * @return SEND_TO_CLIENT_EXCEPTION -22
 *         WRITER_CACHE_INVALID_EXCEPTION -33
 *         CACHE_INVALID_EXCEPTION -44
 *         SUCCESS_WITH_END 11
 */
    int (*sendFromCache)(struct ClientConnection *self, CacheEntry *cache,int *localConnections);
    int (*handleGetRequest)(struct ClientConnection *self, char *buffer, int bufferSize,
                            CacheEntry *cache,
                            const int maxCacheSize,
                            int *localConnectionsCount,
                            int threadId, NodeServerConnection **listServerConnections);

} typedef ClientConnection;

ClientConnection *initClientConnection(int clientSocket);

int handleGettRequest(struct ClientConnection *self, char *buffer, int bufferSize,
                      CacheEntry *cache,
                      const int maxCacheSize,int*localConnectionsCount,
                      int threadId, NodeServerConnection **listServerConnections);


int closeClientConnection(struct ClientConnection *self);

#endif //LAB31_CLIENTCONNECTION_H
