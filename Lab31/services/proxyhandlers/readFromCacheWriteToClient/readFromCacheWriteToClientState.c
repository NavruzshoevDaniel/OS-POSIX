//
// Created by Daniel on 26.12.2020.
//

#include "readFromCacheWriteToClientState.h"
#include "../../logger/logging.h"

int sendNewChunksToClient(Connection *connection, CacheInfo *cache, size_t newSize) {

    for (size_t k = connection->numChunksWritten; k < newSize; k++) {
        infoPrintf("sendNewChunksToClient...");
        pthread_mutex_lock(&cache[connection->cacheIndex].mutex);
        ssize_t bytesWritten = send(connection->clientSocket,
                                    cache[connection->cacheIndex].data[k],
                                    cache[connection->cacheIndex].dataChunksSize[k], MSG_DONTWAIT);
        pthread_mutex_unlock(&cache[connection->cacheIndex].mutex);
        infoPrintf("sendNewChunksToClientEnd\n");
        if (bytesWritten <= 0) {
            perror("Error client from cache sending");
            printf("wht fuck\n");
            return -1;
        }
    }
    return 0;
}

/**
 * @return EXIT_SUCCES 0
 *         SEND_TO_CLIENT_EXCEPTION -2
 *         WRITER_CACHE_INVALID_EXCEPTION -3
 *         CACHE_INVALID_EXCEPTION -4
 *         SUCCESS_WITH_END 1
 * */
int handleReadFromCacheWriteToClientState(Connection *connection,
                                          struct pollfd clientFds,
                                          CacheInfo *cache,
                                          const int *localConnectionsCount) {
    if (clientFds.revents & POLLOUT) {
        int localCacheStatus;
        size_t localNumChunks;

        localCacheStatus = getCacheStatus(&cache[connection->cacheIndex]);
#ifdef _MULTITHREAD
        if (localCacheStatus == VALID || localCacheStatus == DOWNLOADING) {
            infoPrintf("numChunksMutex in handle...");
            pthread_mutex_lock(&cache[connection->cacheIndex].numChunksMutex);

            localNumChunks = cache[connection->cacheIndex].numChunks;

            while (localCacheStatus == DOWNLOADING && connection->numChunksWritten == localNumChunks &&
                   *localConnectionsCount == 1) {
                infoPrintf("\t\tnumChunksMutex before in handle...");
                pthread_cond_wait(&cache[connection->cacheIndex].numChunksCondVar,
                                  &cache[connection->cacheIndex].numChunksMutex);
                infoPrintf("\t\tnumChunksMutex after in handle\n");
                localCacheStatus = getCacheStatus(&cache[connection->cacheIndex]);
                if (localCacheStatus == INVALID) {
                    pthread_mutex_unlock(&cache[connection->cacheIndex].numChunksMutex);
                    return WRITER_CACHE_INVALID_EXCEPTION;
                }
                localNumChunks = cache[connection->cacheIndex].numChunks;
            }

            pthread_mutex_unlock(&cache[connection->cacheIndex].numChunksMutex);
            infoPrintf("numChunksMutex after in handle\n");
            // printf("after numChunksMutex\n");
            if (sendNewChunksToClient(connection, cache, localNumChunks) == -1) {
                return SEND_TO_CLIENT_EXCEPTION;
            }
            connection->numChunksWritten = localNumChunks;

            if (localCacheStatus == VALID && connection->numChunksWritten == cache[connection->cacheIndex].numChunks) {
                return SUCCESS_WITH_END;
            }
#else
            if (localCacheStatus == VALID || localCacheStatus == DOWNLOADING) {

                int totalNumChunks = cache[connection->cacheIndex].numChunks;
                size_t nowChunksWritten = connection->numChunksWritten;
                if (nowChunksWritten < totalNumChunks) {
                    //printf("%s\n\n\n", cache[connections[i].cacheIndex].data[nowChunksWritten]);
                    ssize_t bytesWritten = send(connection->clientSocket,
                                                cache[connection->cacheIndex].data[nowChunksWritten],
                                                cache[connection->cacheIndex].dataChunksSize[nowChunksWritten], MSG_DONTWAIT);
                    printf("%d\n", bytesWritten);
                    if (bytesWritten < 0) {
                        return SEND_TO_CLIENT_EXCEPTION;
                    }
                    connection->numChunksWritten++;
                }

                if (localCacheStatus == VALID && connection->numChunksWritten == totalNumChunks) {
                    return SUCCESS_WITH_END;
                }
#endif
            return EXIT_SUCCESS;
        } else if (localCacheStatus == INVALID) {
            return CACHE_INVALID_EXCEPTION;
        }
    }
}

