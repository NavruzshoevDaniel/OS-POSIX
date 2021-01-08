//
// Created by Daniel on 03.01.2021.
//
#include "clientConnection.h"

int sendFromCache(struct ClientConnection *self, CacheEntry *cacheList, int *localConnections);

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
    int urlInCacheResult = searchUrlInCacheConncurrent(url, cache, maxCacheSize);
    if (urlInCacheResult >= 0) {
        clientConnection->cacheIndex = urlInCacheResult;
        clientConnection->curData = &cache[urlInCacheResult].data->head;
    } else {
        int freeCacheIndex = searchFreeCacheConcurrent(url, cache, maxCacheSize, threadId);
        if ((-1 != freeCacheIndex) ||
            (-1 != (freeCacheIndex =
                            searchNotUsingCacheConcurrent(url, cache, maxCacheSize, threadId)))) {

            int serverSocket = getServerSocketBy(url);
            if (serverSocket == -1) {
                handleNotResolvingUrl(serverSocket);
                free(url);
                return RESOLVING_SOCKET_FROM_URL_EXCEPTION;
            }
            ServerConnection *serverConnection = initServerConnection(serverSocket, freeCacheIndex);
            serverConnection->cacheIndex = freeCacheIndex;
            clientConnection->cacheIndex = freeCacheIndex;
            clientConnection->curData = &cache[freeCacheIndex].data->head;
            char *data = createGet(url, &bufferLength);
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
    printf("handleGettRequest:%s\n", buffer);
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
    if (self->clientSocket != -1) {
        int closeRes = close(self->clientSocket);
        if (closeRes != 0) {
            perror("close");
        }
    }
    free(self);
    return EXIT_SUCCESS;
}

int sendNewChunksToClientt(ClientConnection *connection, CacheEntry *cache, size_t newSize) {
    NodeCacheData *cacheData = *connection->curData;
    int counter = connection->numChunksWritten;

    while (cacheData != NULL && (counter < newSize)) {
        ssize_t bytesWritten = send(connection->clientSocket, cacheData->data, cacheData->lengthData, MSG_DONTWAIT);
        if (bytesWritten <= 0) {
            perror("Error client from cache sending");
            return -1;
        }
        connection->curData = &cacheData->next;
        cacheData = cacheData->next;

        counter++;
    }
    return 0;
}

int sendFromCache(struct ClientConnection *self, CacheEntry *cache, int *localConnections) {
    int localCacheStatus;
    size_t localNumChunks;

    localCacheStatus = getCacheStatus(&cache[self->cacheIndex]);
    if (localCacheStatus == VALID || localCacheStatus == DOWNLOADING) {
        warnPrintf("numChunksMutex before in handle...");
        pthread_mutex_lock(&cache[self->cacheIndex].numChunksMutex);

        localNumChunks = cache[self->cacheIndex].numChunks;

        while (localCacheStatus == DOWNLOADING && self->numChunksWritten == localNumChunks
               && *localConnections == 1) {
            warnPrintf("\t\tnumChunksMutex pthread_cond_wait before in handle...");
            pthread_cond_wait(&cache[self->cacheIndex].numChunksCondVar,
                              &cache[self->cacheIndex].numChunksMutex);
            warnPrintf("\t\tnumChunksMutex pthread_cond_wait after in handle");
            localCacheStatus = getCacheStatus(&cache[self->cacheIndex]);
            if (localCacheStatus == INVALID) {
                pthread_mutex_unlock(&cache[self->cacheIndex].numChunksMutex);
                warnPrintf("numChunksMutex WRITER_CACHE_INVALID_EXCEPTION before in handle");
                return WRITER_CACHE_INVALID_EXCEPTION;
            }
            localNumChunks = cache[self->cacheIndex].numChunks;
        }

        pthread_mutex_unlock(&cache[self->cacheIndex].numChunksMutex);
        warnPrintf("numChunksMutex before in handle");
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
