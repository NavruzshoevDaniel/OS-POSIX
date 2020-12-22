//
// Created by Daniel on 20.12.2020.
//

#include <poll.h>
#include "../../connection/connection.h"
#include "../../http/httpService.h"
#include "../../cache/cache.h"
/*

void handleNotGetMethod(struct Connection *connection) {
    char wrong[] = "HTTP: 405\r\nAllow: GET\r\n";
    write(connection->clientSocket, wrong, 23);
}

void handleNotResolvingUrl(struct Connection *connection) {
    char errorstr[] = "HTTP: 523\r\n";
    write(connection->clientSocket, errorstr, 11);
}

void handleGetMethod(char *url, Connection *connection, int *localConnectionsCount, int threadId) {
    int urlInCacheResult = searchUrlInCache(url, cache, MAX_CACHE_SIZE);
    if (urlInCacheResult >= 0) {
        setReadFromCacheState(connection, urlInCacheResult);
    } else {
        int freeCacheIndex = searchFreeCacheAndSetDownloadingState(url, cache, MAX_CACHE_SIZE, threadId);
        if ((-1 != freeCacheIndex) ||
            (-1 != (freeCacheIndex =
                            searchNotUsingCacheAndSetDownloadingState(url, cache, MAX_CACHE_SIZE, threadId)))) {

            setWriteToServerState(connection, freeCacheIndex);
            connection->serverSocket = getServerSocketBy(url);
            free(connection->buffer);

            connection->buffer = createGet(url, &connection->buffer_size);

            if (connection->serverSocket == -1) {
                handleNotResolvingUrl(connection);
                dropConnectionWrapper(i, "CLIENT_MESSAGE:get server err", 0,
                                      connection, localConnectionsCount, threadId);
                free(url);
                return;
            }
        } else {
            setWriteToServerState(connection, -1);
        }
    }
}

void handleGettingRequestState(Connection *connection,
                               char *buf,
                               int threadId,
                               struct pollfd clientFds) {
    if (clientFds.revents & POLLHUP) {
        dropConnectionWrapper(i, "CLIENT_MESSAGE:dead client ", 0,
                              connection, localConnectionsCount, threadId);
    } else if (clientFds.revents & POLLIN) {
        ssize_t readCount = recv(connection->clientSocket, buf, BUFFER_SIZE, 0);

        if (readCount <= 0) {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:recv err", 0, connection,
                                  localConnectionsCount, threadId);
            return;
        }
        int bufferErr;
        if (isConnectionBufferEmpty(connection)) {
            bufferErr = allocateConnectionBufferMemory(connection, readCount);
        } else {
            printf("REALLOCATE CACHE");
            //TODO::this is really need?
            bufferErr = reallocateConnectionBufferMemory(connection, readCount);
        }

        if (bufferErr == -1) {
            dropConnectionWrapper(i, "buffer error",
                                  0, connection, localConnectionsCount, threadId);
            return;
        }

        memcpy(connection->buffer, buf, (size_t) readCount);

        if (connection->buffer_size > 3) {
            char *url = getUrlFromData(connection->buffer);

            if (url != NULL) {
                if (!isMethodGet(connection->buffer)) {
                    handleNotGetMethod(connection);
                    dropConnectionWrapper(i, "CLIENT_MESSAGE:not GET", 0,
                                          connection, localConnectionsCount, threadId);
                    free(url);
                } else {
                    handleGetMethod(url, connection, i, localConnectionsCount, threadId);
                }
            } else {
                dropConnectionWrapper(i, "CLIENT_MESSAGE:not good url", 0, connection,
                                      localConnectionsCount, threadId);
            }
        }
    }
}
*/

