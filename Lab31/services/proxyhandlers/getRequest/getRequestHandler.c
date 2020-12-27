//
// Created by Daniel on 20.12.2020.
//

#include "getRequestHandler.h"


void handleNotGetMethod(struct Connection *connection) {
    char wrong[] = "HTTP: 405\r\nAllow: GET\r\n";
    write(connection->clientSocket, wrong, 23);
}

void handleNotResolvingUrl(struct Connection *connection) {
    char errorstr[] = "HTTP: 523\r\n";
    write(connection->clientSocket, errorstr, 11);
}

/**
 * @return 0 - SUCCESS
 *         RESOLVING_SOCKET_FROM_URL_EXCEPTION -6
 * */
int handleGetMethod(char *url,
                    Connection *connection,
                    CacheInfo *cache,
                    const int maxCacheSize,
                    int threadId) {
    int urlInCacheResult = searchUrlInCache(url, cache, maxCacheSize);
    if (urlInCacheResult >= 0) {
        setReadFromCacheState(connection, urlInCacheResult);
    } else {
        int freeCacheIndex = searchFreeCacheAndSetDownloadingState(url, cache, maxCacheSize, threadId);
        if ((-1 != freeCacheIndex) ||
            (-1 != (freeCacheIndex =
                            searchNotUsingCacheAndSetDownloadingState(url, cache, maxCacheSize, threadId)))) {

            setWriteToServerState(connection, freeCacheIndex);
            connection->serverSocket = getServerSocketBy(url);
            free(connection->buffer);

            connection->buffer = createGet(url, &connection->buffer_size);

            if (connection->serverSocket == -1) {
                handleNotResolvingUrl(connection);
                free(url);
                return RESOLVING_SOCKET_FROM_URL_EXCEPTION;
            }
        } else {
            setWriteToServerState(connection, -1);
        }
    }
}

/**
 * @return 0 - success
 *          DEAD_CLIENT_EXCEPTION -2
 *          RECV_CLIENT_EXCEPTION -3
 *          ALLOCATING_BUFFER_MEMORY_EXCEPTION -4
 *          NOT_GET_EXCEPTION -5
 *          URL_EXCEPTION -6
 * */
int handleGettingRequestState(Connection *connection,
                              char *buf,
                              const int bufferSize,
                              int threadId,
                              struct pollfd clientFds,
                              CacheInfo *cache,
                              const int maxCacheSize) {
    if (clientFds.revents & POLLHUP) {
        return DEAD_CLIENT_EXCEPTION;
    } else if (clientFds.revents & POLLIN) {
        ssize_t readCount = recv(connection->clientSocket, buf, bufferSize, 0);

        if (readCount <= 0) { return RECV_CLIENT_EXCEPTION; }
        int bufferErr;
        if (isConnectionBufferEmpty(connection)) {
            bufferErr = allocateConnectionBufferMemory(connection, readCount + 1);
        } else {
            bufferErr = reallocateConnectionBufferMemory(connection, readCount);
        }

        if (bufferErr == -1) { return ALLOCATING_BUFFER_MEMORY_EXCEPTION; }

        memcpy(connection->buffer, buf, (size_t) readCount);

        if (connection->buffer_size > 3) {
            char *url = getUrlFromData(connection->buffer);

            if (url != NULL) {
                if (!isMethodGet(connection->buffer)) {
                    handleNotGetMethod(connection);
                    free(url);
                    return NOT_GET_EXCEPTION;
                } else {
                    int result = handleGetMethod(url, connection, cache, maxCacheSize, threadId);
                    if (result == RESOLVING_SOCKET_FROM_URL_EXCEPTION) {
                        return RESOLVING_SOCKET_FROM_URL_EXCEPTION;
                    }
                }
            } else {
                return URL_EXCEPTION;
            }
        }
    }
}


