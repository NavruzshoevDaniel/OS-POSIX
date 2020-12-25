//
// Created by Daniel on 20.12.2020.
//

#include "cache.h"

void freeDataChunks(char **data, size_t numChunks) {

    for (size_t i = 0; i < numChunks; i++) {
        free(data[i]);
    }
    printf("free data\n");
    free(data);
}

/**
 * if url exits set READ_FROM_CACHE_WRITE_CLIENT state connection and return index cache
 * or else return -1
 * */
int searchUrlInCache(char *url, CacheInfo *cache, int cacheSize) {
    for (int j = 0; j < cacheSize; j++) {

        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].url != NULL) {
            //printf("(%s)\n(%s)\n", cache[j].url,url);
            //printf("%d\n", strcmp(cache[j].url, url));
        }

        if (cache[j].url != NULL && strcmp(cache[j].url, url) == 0) {
            if (cache[j].status == VALID || cache[j].status == DOWNLOADING) {
                //printf("valid download\n");
                cache[j].readers++;
                pthread_mutex_unlock(&cache[j].mutex);
                return j;
            }

            pthread_mutex_unlock(&cache[j].mutex);
        } else { pthread_mutex_unlock(&cache[j].mutex); }
    }
    return -1;
}

/**
 * if free cache exits set WRITE_TO_SERVER state connection and return index cache
 * or else return -1
 * */
int searchFreeCacheAndSetDownloadingState(char *url,
                                          CacheInfo *cache,
                                          int cacheSize,
                                          int threadId) {
    for (int j = 0; j < cacheSize; j++) {

        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].url == NULL) {
            printf("(%d)SEARCH_CACHE: found free cache id=%d\n", threadId, j);
            cache[j].readers = 1;
            cache[j].status = DOWNLOADING;
            cache[j].writerId = threadId;
            cache[j].data = NULL;
            cache[j].dataChunksSize = NULL;
            cache[j].numChunks = 0;
            cache[j].allSize = 0;
            cache[j].recvSize = 0;
            cache[j].url = (char *) malloc(sizeof(char) * strlen(url) + 1);
            memcpy(cache[j].url, url, sizeof(char) * strlen(url) + 1);

            pthread_mutex_unlock(&cache[j].mutex);
            return j;
        } else { pthread_mutex_unlock(&cache[j].mutex); }
    }
    return -1;
}

/**
 * if not using cache exits set WRITE_TO_SERVER state connection and return index cache
 * or else return -1
 * */
int searchNotUsingCacheAndSetDownloadingState(char *url,
                                              CacheInfo *cache,
                                              int cacheSize,
                                              int threadId) {

    //TODO::whts different between this method and searchFreeCacheAndSetDownloadingState method
    for (int j = 0; j < cacheSize; j++) {

        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].readers == 0 || cache[j].status == INVALID) {
            printf("(%d)SEARCH_CACHE: found not using cache id=%d\n", threadId, j);
            cache[j].readers = 1;
            cache[j].status = DOWNLOADING;
            cache[j].writerId = threadId;
            cache[j].data = NULL;
            cache[j].dataChunksSize = NULL;
            cache[j].numChunks = 0;
            cache[j].allSize = 0;
            cache[j].recvSize = 0;

            freeDataChunks(cache[j].data, cache[j].numChunks);
            free(cache[j].dataChunksSize);
            free(cache[j].url);
            cache[j].url = (char *) malloc(sizeof(char) * sizeof(url));
            memcpy(cache[j].url, url, sizeof(char) * sizeof(url));

            pthread_mutex_unlock(&cache[j].mutex);
            return j;
        } else { pthread_mutex_unlock(&cache[j].mutex); }
    }
    return -1;
}

void makeCacheInvalid(CacheInfo *cache) {
    pthread_mutex_lock(&cache->mutex);
    cache->status = INVALID;
    cache->writerId = -1;
    pthread_mutex_unlock(&cache->mutex);
    pthread_cond_broadcast(&cache->numChunksCondVar);
}

int initCache(CacheInfo *cache, const int maxCacheSize) {

    bool erMS, erCVC, erMC;
    for (int i = 0; i < maxCacheSize; i++) {

        cache[i].allSize = 0;
        cache[i].recvSize = 0;
        erMS = initMutex(&cache[i].mutex);

        cache[i].readers = 0;
        cache[i].data = NULL;
        cache[i].dataChunksSize = NULL;
        cache[i].numChunks = 0;
        erCVC = initCondVariable(&cache[i].numChunksCondVar);
        erMC = initMutex(&cache[i].numChunksMutex);

        cache[i].writerId = -1;
        cache[i].url = NULL;

    }
    return erMS && erCVC && erMC;
}

void destroyCache(CacheInfo *cache, const int maxCacheSize) {

    for (int i = 0; i < maxCacheSize; i++) {

        pthread_mutex_destroy(&cache[i].mutex);
        freeDataChunks(cache[i].data, cache[i].numChunks);
        pthread_cond_destroy(&cache[i].numChunksCondVar);
        pthread_mutex_destroy(&cache[i].numChunksMutex);

        free(cache[i].url);
    }
    printf("destroy cache\n");
}

int putDataToCache(CacheInfo *cacheChunk, char *newData, int lengthNewData) {

    char **reallocatedCacheData = (char **) realloc(cacheChunk->data,
                                                    (cacheChunk->numChunks + 1) * sizeof(char *));
    if (reallocatedCacheData == NULL) {
        printf("CACHE malloc failed\n");
        makeCacheInvalid(cacheChunk);
        return -1;
    }
    cacheChunk->data = reallocatedCacheData;

    int *realocatedDataChunksSize = (int *) realloc(cacheChunk->dataChunksSize,
                                                    (cacheChunk->numChunks + 1) * sizeof(int));
    if (realocatedDataChunksSize == NULL) {
        //free old data
        printf("CACHE malloc failed\n");
        makeCacheInvalid(cacheChunk);
        return -1;
    }
    cacheChunk->dataChunksSize = realocatedDataChunksSize;
    cacheChunk->dataChunksSize[cacheChunk->numChunks] = sizeof(char) * lengthNewData;

    cacheChunk->data[cacheChunk->numChunks] = (char *) malloc(sizeof(char) * lengthNewData);

    if (cacheChunk->data[cacheChunk->numChunks] == NULL) {
        printf("CACHE malloc failed\n");
        makeCacheInvalid(cacheChunk);
        return -1;
    }

    pthread_mutex_lock(&cacheChunk->mutex);
    cacheChunk->recvSize += lengthNewData;

    memcpy(cacheChunk->data[cacheChunk->numChunks], newData, sizeof(char) * lengthNewData);

    pthread_mutex_unlock(&cacheChunk->mutex);

    pthread_mutex_lock(&cacheChunk->numChunksMutex);
    cacheChunk->numChunks++;
    pthread_mutex_unlock(&cacheChunk->numChunksMutex);
    return 0;
}


int broadcastWaitingCacheClients(CacheInfo *cacheChunk) {
    pthread_cond_broadcast(&cacheChunk->numChunksCondVar);
}