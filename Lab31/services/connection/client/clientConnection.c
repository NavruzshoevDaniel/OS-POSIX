//
// Created by Daniel on 03.01.2021.
//
#include <logger/logging.h>
#include <sys/socket.h>
#include <net/httpService.h>
#include "clientConnection.h"

int sendFromCache(struct ClientConnection *self, CacheEntry *cacheList, const int *localConnections);

ClientConnection *initClientConnection(int clientSocket) {
    ClientConnection *outNewClientConnection = malloc(sizeof(ClientConnection));
    outNewClientConnection->clientSocket = clientSocket;
    outNewClientConnection->numChunksWritten = 0;
    outNewClientConnection->state = WAITING_REQUEST;
    outNewClientConnection->id = rand() % 9000 + 1000;

    outNewClientConnection->sendFromCache = &sendFromCache;
    outNewClientConnection->handleGetRequest = &handleGettRequest;
    return outNewClientConnection;
}

void handleNotGetMethod(ClientConnection *connection) {
    char wrong[] = "HTTP: 405\r\nAllow: GET\r\n";
    write(connection->clientSocket, wrong, 23);
}

void handleNotResolvingUrl(int clientSocket) {
    char errorstr[] = "HTTP: 523\r\n";
    write(clientSocket, errorstr, 11);
}

int handleGetMethod(ClientConnection *clientConnection, char *url,
                    CacheEntry *cache,
                    const int maxCacheSize,
                    int threadId,
                    char *buf,
                    size_t bufferLength,
                    int *localConnectionsCount,
                    NodeServerConnection **listServerConnections) {
    int urlInCacheResult = searchUrlInCache(url, cache, maxCacheSize);
    if (urlInCacheResult >= 0) {
        clientConnection->cacheIndex = urlInCacheResult;
    } else {
        int freeCacheIndex = searchFreeCacheAndSetDownloadingState(url, cache, maxCacheSize, threadId);
        if ((-1 != freeCacheIndex) ||
            (-1 != (freeCacheIndex =
                            searchNotUsingCacheAndSetDownloadingState(url, cache, maxCacheSize, threadId)))) {

            int serverSocket = getServerSocketBy(url);
            if (serverSocket == -1) {
                handleNotResolvingUrl(serverSocket);
                free(url);
                return RESOLVING_SOCKET_FROM_URL_EXCEPTION;
            }
            ServerConnection *serverConnection = initServerConnection(serverSocket, freeCacheIndex);
            serverConnection->cacheIndex = freeCacheIndex;
            clientConnection->cacheIndex = freeCacheIndex;
            char *data = createGet(url, &bufferLength);
            printf("data=%s\n", data);
            int result = serverConnection->sendRequest(serverConnection, data, bufferLength);
            (*localConnectionsCount)++;
            if (result != 0) {
                printf("salam\n");
                return -1;
            }
            pushServerConnectionBack(listServerConnections, serverConnection);
        }
    }
    return 0;

}

int handleGettRequest(struct ClientConnection *self, char *buffer, int bufferSize,
                      CacheEntry *cache,
                      const int maxCacheSize, int *localConnectionsCount,
                      int threadId, NodeServerConnection **listServerConnections) {

    ssize_t readCount = recv(self->clientSocket, buffer, bufferSize, 0);
    printf("handleGettRequest:%s\n",buffer);
    if (readCount <= 0) { return RECV_CLIENT_EXCEPTION; }
    if (readCount > 3) {
        char *url = getUrlFromData(buffer);

        if (url != NULL) {
            if (!isMethodGet(buffer)) {
                handleNotGetMethod(self);
                free(url);
                return NOT_GET_EXCEPTION;
            } else {
                int result = handleGetMethod(self, url, cache, maxCacheSize, threadId, buffer, readCount,
                                             localConnectionsCount,
                                             listServerConnections);
                if (result == RESOLVING_SOCKET_FROM_URL_EXCEPTION) {
                    return RESOLVING_SOCKET_FROM_URL_EXCEPTION;
                }
            }
        } else {
            return URL_EXCEPTION;
        }
    }
    return EXIT_SUCCESS;
}

int closeClientConnection(struct ClientConnection *self) {
    close(self->clientSocket);
    free(self);
    return EXIT_SUCCESS;
}

int sendNewChunksToClientt(ClientConnection *connection, CacheEntry *cache, size_t newSize) {
    NodeCacheData *cacheData = getCacheNode(cache->data, connection->numChunksWritten);
    int counter = connection->numChunksWritten;
    while (cacheData != NULL && (counter < newSize)) {
        ssize_t bytesWritten = send(connection->clientSocket, cacheData->data, cacheData->lengthData, MSG_DONTWAIT);
        if (bytesWritten <= 0) {
            perror("Error client from cache sending");
            printf("wht fuck\n");
            return -1;
        }
        cacheData = cacheData->next;
        counter++;
    }
    return 0;
}

int sendFromCache(struct ClientConnection *self, CacheEntry *cache, const int *localConnections) {
    int localCacheStatus;
    size_t localNumChunks;

    localCacheStatus = getCacheStatus(&cache[self->cacheIndex]);
    if (localCacheStatus == VALID || localCacheStatus == DOWNLOADING) {
        //infoPrintf("numChunksMutex in handle...");
        pthread_mutex_lock(&cache[self->cacheIndex].numChunksMutex);

        localNumChunks = cache[self->cacheIndex].numChunks;

        while (localCacheStatus == DOWNLOADING && self->numChunksWritten == localNumChunks
               && *localConnections == 1) {
            //infoPrintf("\t\tnumChunksMutex before in handle...");
            pthread_cond_wait(&cache[self->cacheIndex].numChunksCondVar,
                              &cache[self->cacheIndex].numChunksMutex);
            // infoPrintf("\t\tnumChunksMutex after in handle");
            localCacheStatus = getCacheStatus(&cache[self->cacheIndex]);
            if (localCacheStatus == INVALID) {
                pthread_mutex_unlock(&cache[self->cacheIndex].numChunksMutex);
                return WRITER_CACHE_INVALID_EXCEPTION;
            }
            localNumChunks = cache[self->cacheIndex].numChunks;
        }

        pthread_mutex_unlock(&cache[self->cacheIndex].numChunksMutex);
        //infoPrintf("numChunksMutex after in handle\n");
        // printf("after numChunksMutex\n");
        if (sendNewChunksToClientt(self, &cache[self->cacheIndex], localNumChunks) == -1) {
            return SEND_TO_CLIENT_EXCEPTION;
        }
        self->numChunksWritten = localNumChunks;

        if (localCacheStatus == VALID && self->numChunksWritten == cache[self->cacheIndex].numChunks) {
            return SUCCESS_WITH_END;
        }
    } else {
        return CACHE_INVALID_EXCEPTION;
    }
    return EXIT_SUCCESS;
}
