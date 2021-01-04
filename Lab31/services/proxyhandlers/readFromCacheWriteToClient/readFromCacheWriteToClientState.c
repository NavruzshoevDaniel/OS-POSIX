//
// Created by Daniel on 26.12.2020.
//

#include "readFromCacheWriteToClientState.h"
#include "../../logger/logging.h"

int sendNewChunksToClient(Connection *connection, CacheEntry *cache, size_t newSize) {
    NodeCacheData *cacheData = getCacheNode(cache->data, connection->numChunksWritten);
    int counter = connection->numChunksWritten;
    while (cacheData != NULL && (counter < newSize)) {
        ssize_t bytesWritten = send(connection->clientSocket, cacheData->data,cacheData->lengthData, MSG_DONTWAIT);
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

bool isClientDeadd(struct pollfd clientFd) {
    return clientFd.revents & POLLERR || clientFd.revents & POLLHUP || clientFd.revents & POLLNVAL;
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
                                          CacheEntry *cache,
                                          const int *localConnectionsCount) {
    if (isClientDeadd(clientFds)) {
        return SEND_TO_CLIENT_EXCEPTION;
    }
    if (clientFds.revents & POLLOUT) {
        int localCacheStatus;
        size_t localNumChunks;

        localCacheStatus = getCacheStatus(&cache[connection->cacheIndex]);
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
            return EXIT_SUCCESS;
        } else if (localCacheStatus == INVALID) {
            return CACHE_INVALID_EXCEPTION;
        }
    }
}
