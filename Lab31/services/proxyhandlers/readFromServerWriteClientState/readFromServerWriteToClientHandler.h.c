//
// Created by Daniel on 23.12.2020.
//
#include "readFromServerWriteToClientHandler.h"


bool isFirstCacheChunk(const CacheInfo *cache) {
    return DOWNLOADING == cache->status && cache->recvSize == 0;
}

/**
 * @return 0 - SUCCESS
 *         EMPTY_METHOD -1
 *         RECV_FROM_SERVER_EXCEPTION -2
 *         SERVER_CLOSED_EXCEPTION -3
 *         STATUS_OR_CONTENT_LENGTH_EXCEPTION -4
 *         BODY_HTTP_EXCEPTION -5
 *         NOT_FREE_CACHE_EXCEPTION -6
 *         END_READING_PROCCESS 1
 *
 * */
int handleReadFromServerWriteToClientState(Connection *connection,
                                           struct pollfd clientFd,
                                           struct pollfd serverFd,
                                           CacheInfo *cache,
                                           char *buf,
                                           int bufferSize,
                                           int threadId) {
    if ((clientFd.revents & POLLOUT && serverFd.revents & POLLIN) ||
        (connection->clientSocket == -1 && serverFd.revents & POLLIN)) {

        ssize_t readCount = recv(connection->serverSocket, buf, bufferSize, 0);

        if (readCount < 0) { return RECV_FROM_SERVER_EXCEPTION; }
        if (readCount == 0) { return SERVER_CLOSED_EXCEPTION; }

        if (connection->clientSocket != -1) {
            if (send(connection->clientSocket, buf, (size_t) readCount, 0) <= 0) {
                printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT: CLIENT ERROR\n", threadId,
                       connection->id);
                connection->clientSocket = -1;
            }
        }

        if (connection->cacheIndex == -1) { return NOT_FREE_CACHE_EXCEPTION; }

        if (isFirstCacheChunk(&cache[connection->cacheIndex])) {
            char *dest = buf;
            int body = getIndexOfBody(dest, readCount);
            if (body == -1) { return BODY_HTTP_EXCEPTION; }

            int statusCode = getStatusCodeAnswer(dest);
            long contentLength = getContentLengthFromAnswer(dest);

            if (statusCode != 200 || contentLength == -1) { return STATUS_OR_CONTENT_LENGTH_EXCEPTION; }
            cache[connection->cacheIndex].allSize = (size_t) (contentLength + body);
        }

        putDataToCache(&cache[connection->cacheIndex], buf, readCount);

        broadcastWaitingCacheClients(&cache[connection->cacheIndex]);

        if (cache[connection->cacheIndex].recvSize == cache[connection->cacheIndex].allSize) {
            pthread_mutex_lock(&cache[connection->cacheIndex].mutex);
            cache[connection->cacheIndex].status = VALID;
            pthread_mutex_unlock(&cache[connection->cacheIndex].mutex);
            return END_READING_PROCCESS;
        }
        return 0;
    }
    return -1;
}