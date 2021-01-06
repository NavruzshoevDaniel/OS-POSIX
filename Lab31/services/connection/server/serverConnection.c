//
// Created by Daniel on 03.01.2021.
//
#include "serverConnection.h"

int sendRequest(struct ServerConnection *self, char *data, int dataSize);

int caching(struct ServerConnection *self, CacheEntry *cache, void *buf, size_t bufferSize);

ServerConnection *initServerConnection(int serverSocket, int cacheIndex) {
    ServerConnection *outNewServerConnection = (ServerConnection *) malloc(sizeof(ServerConnection));

    outNewServerConnection->serverSocket = serverSocket;
    outNewServerConnection->cacheIndex = cacheIndex;
    outNewServerConnection->state = REQUEST_SENDING;
    outNewServerConnection->id = rand() % 9000 + 1000;

    outNewServerConnection->sendRequest = &sendRequest;
    outNewServerConnection->caching = &caching;
    return outNewServerConnection;
}

int sendRequest(struct ServerConnection *self, char *data, int dataSize) {
    if (send(self->serverSocket, data, dataSize, MSG_DONTWAIT) <= 0) {
        perror("Send to server");
        return -1;
    }
    self->state = CACHING;
    return EXIT_SUCCESS;
}

bool isFirstCacheChunkk(CacheEntry *cache) {
    return DOWNLOADING == getCacheStatus(cache) && getCacheRecvSize(cache) == 0;
}

int caching(struct ServerConnection *self, CacheEntry *cache, void *buf, size_t bufferSize) {

    ssize_t readCount = recv(self->serverSocket, buf, bufferSize, 0);

    if (readCount < 0) {
        broadcastWaitingCacheClients(cache);
        return RECV_FROM_SERVER_EXCEPTION;
    }
    if (readCount == 0) {
        broadcastWaitingCacheClients(cache);
        return SERVER_CLOSED_EXCEPTION;
    }

    if (isFirstCacheChunkk(cache)) {
        char *dest = buf;
        int body = getIndexOfBody(dest, readCount);

        int statusCode = getStatusCodeAnswer(dest);
        long contentLength = getContentLengthFromAnswer(dest);

        if (statusCode != 200 || (contentLength == -1 && body == -1)) {
            printf("status=%d\n",statusCode);
            printf("contentLength=%ld\n",contentLength);
            printf("body=%d\n",body);
            broadcastWaitingCacheClients(cache);
            return STATUS_OR_CONTENT_LENGTH_EXCEPTION;
        }
        printf("Responce:%s\n",buf);
        setCacheAllSize(cache, (size_t) (contentLength + body));
    }
    if (putDataToCache(cache, buf, readCount) == -1) {
        broadcastWaitingCacheClients(cache);
        return PUT_CACHE_DATA_EXCEPTION;
    }

    broadcastWaitingCacheClients(cache);

    if (getCacheRecvSize(cache) == getCacheAllSize(cache)) {
        setCacheStatus(cache, VALID);
        return END_READING_PROCCESS;
    }
    return EXIT_SUCCESS;
}

int closeServerConnection(struct ServerConnection *self) {
    if (self->serverSocket!=-1){
        close(self->serverSocket);
        self->serverSocket=-1;
    }
    free(self);
    return EXIT_SUCCESS;
}