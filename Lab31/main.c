#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include "services/queue/queueService.h"
#include "services/connection/connection.h"
#include "services/threadpool/threadPool.h"
#include "argschecker/argsChecker.h"
#include "services/concurrent/atomicInt.h"
#include "services/cache/cache.h"
#include "services/http/httpService.h"

#define MAX_CONNECTIONS 100
#define MAX_CACHE_SIZE 3*1024
#define BUFFER_SIZE 16 * 1024
#define MAX_NUM_TRANSLATION_CONNECTIONS 100

//3 = CRLF EOF

Queue *socketsQueue;
static int allConnectionsCount = 0;
int poolSize;

CacheInfo cache[MAX_CACHE_SIZE];

pthread_mutex_t connectionsMutex;
//----------------------------------------------------------------------------------SOCKET
int getProxySocket(int port) {

    struct sockaddr_in listenAddress;

    listenAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    listenAddress.sin_family = AF_INET;
    listenAddress.sin_port = htons(port);

    int proxySocket = socket(AF_INET, SOCK_STREAM, 0);

    if (proxySocket < 0) {
        perror("Cannot create proxySocket");
        exit(EXIT_FAILURE);
    }

    if (bind(proxySocket, (struct sockaddr *) &listenAddress, sizeof(listenAddress))) {
        perror("Cannot bind proxySocket");
        exit(EXIT_FAILURE);
    }

    if (listen(proxySocket, MAX_NUM_TRANSLATION_CONNECTIONS)) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    return proxySocket;
}

int getServerSocket(char *url) {

    char *host = getHostFromUrl(url);

    struct hostent *hostInfo = gethostbyname(host);

    if (NULL == hostInfo) {
        fprintf(stderr, "Cannot get host by name\n");
        return -1;
    }

    struct sockaddr_in destinationAddress;

    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = htons(80);
    memcpy(&destinationAddress.sin_addr, hostInfo->h_addr, hostInfo->h_length);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == serverSocket) {
        perror("Cannot create socket");
        return -1;
    }

    if (-1 == connect(serverSocket, (struct sockaddr *) &destinationAddress, sizeof(destinationAddress))) {
        perror("Cannot connect");
        return -1;
    }

    return serverSocket;
}

void wrongMethod(struct Connection *connection) {
    char wrong[] = "HTTP: 405\r\nAllow: GET\r\n";
    write(connection->clientSocket, wrong, 23);
}

void cannotResolve(struct Connection *connection) {
    char errorstr[] = "HTTP: 523\r\n";
    write(connection->clientSocket, errorstr, 11);
}

//----------------------------------------------------------------------------------ATTR
void updatePoll(struct pollfd *fds, int localCount, Connection *connections) {
    for (int i = 0; i < localCount; ++i) {
        fds[i * 2].fd = connections[i].clientSocket;
        fds[i * 2 + 1].fd = connections[i].serverSocket;
        switch (connections[i].status) {
            case GETTING_REQUEST_FROM_CLIENT:
                fds[i * 2].events = POLLIN;
                fds[i * 2 + 1].events = 0;
                break;
            case WRITE_TO_SERVER:
                fds[i * 2].events = 0;
                fds[i * 2 + 1].events = POLLOUT;
                break;
            case READ_FROM_SERVER_WRITE_CLIENT:
                fds[i * 2].events = POLLOUT;
                fds[i * 2 + 1].events = POLLIN;
                break;
            case READ_FROM_CACHE_WRITE_CLIENT:
                fds[i * 2].events = POLLOUT;
                fds[i * 2 + 1].events = 0;
                break;
            case NOT_ACTIVE:
                break;
        }
    }
}

int getNewClientSocketOrWait(int *localConnectionsCount, int threadId) {
    int newClientSocket = -1;
    printf("want to lock %d\n", threadId);
    pthread_mutex_lock(&socketsQueue->queueMutex);
    printf(" locked %d\n", threadId);
    pthread_mutex_lock(&connectionsMutex);
    if (allConnectionsCount / poolSize >= *localConnectionsCount && socketsQueue->size > 0) {

        newClientSocket = getSocketFromQueue(socketsQueue);
        if (newClientSocket != -1) {
            (*localConnectionsCount)++;
        }
    }
    pthread_mutex_unlock(&connectionsMutex);
    printf("unlocked\n");
    while (*localConnectionsCount == 0 && socketsQueue->size == 0) {
        printf(" unlock and wait\n");
        pthread_cond_wait(&socketsQueue->condVar, &socketsQueue->queueMutex);
        printf(" locked\n");
        newClientSocket = getSocketFromQueue(socketsQueue);
        if (newClientSocket != -1) {
            (*localConnectionsCount)++;
        }
    }
    pthread_mutex_unlock(&socketsQueue->queueMutex);
    return newClientSocket;
}

void dropConnectionWrapper(int id,
                           const char *reason,
                           int server,
                           Connection *connections,
                           int *connectionsCount,
                           int threadId) {
    dropConnection(id, reason, connections, connectionsCount, threadId);
    if (server) {
        close(connections[id].serverSocket);
    }
    atomicDecrement(&allConnectionsCount, &connectionsMutex);
}


void searchingProccess(){

}

void handleGettingRequest(Connection *connections,
                          struct pollfd *fds,
                          int *localConnectionsCount,
                          char *buf,
                          int threadId,
                          int i) {
    if (fds[i * 2].revents & POLLHUP) {
        dropConnectionWrapper(i, "CLIENT_MESSAGE:dead client ", 0,
                              connections, localConnectionsCount, threadId);
    } else if (fds[i * 2].revents & POLLIN) {

        ssize_t readCount = recv(connections[i].clientSocket, buf, BUFFER_SIZE, 0);

        if (readCount <= 0) {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:recv err", 0, connections,
                                  localConnectionsCount, threadId);
            return;
        }
        //TODO::refract
        int bufferErr = 0;
        if (isConnectionBufferEmpty(&connections[i])) {
            bufferErr = allocateConnectionBufferMemory(&connections[i], readCount);
        } else {
            bufferErr = reallocateConnectionBufferMemory(&connections[i], readCount);
        }

        if (bufferErr) {
            dropConnectionWrapper(i, "buffer error",
                                  0, connections, localConnectionsCount, threadId);
            return;
        }

        memcpy(connections[i].buffer, buf, (size_t) readCount);

        if (connections[i].buffer_size > 3) {
            char *url = getUrlFromData(connections[i].buffer);

            if (url != NULL) {
                if (!isMethodGet(connections[i].buffer)) {
                    wrongMethod(&connections[i]);
                    dropConnectionWrapper(i, "CLIENT_MESSAGE:not GET", 0,
                                          connections, localConnectionsCount, threadId);
                    free(url);
                } else {

                    if (searchUrlInCache(url, cache, &connections[i], MAX_CACHE_SIZE) != -1
                        || searchFreeCacheAndSetDownloadingState(url, cache, &connections[i],
                                                                 MAX_CACHE_SIZE, threadId) != -1
                        || searchNotUsingCacheAndSetDownloadingState(url, cache, &connections[i],
                                                                     MAX_CACHE_SIZE, threadId) != -1) {

                        connections[i].serverSocket = getServerSocket(url);
                        free(connections[i].buffer);

                        connections[i].buffer = createGet(url, &connections[i].buffer_size);

                        if (connections[i].serverSocket == -1) {
                            cannotResolve(&connections[i]);
                            dropConnectionWrapper(i, "CLIENT_MESSAGE:get server err", 0,
                                                  connections, localConnectionsCount, threadId);
                            free(url);
                            return;
                        }
                    } else {
                        connections[i].status = WRITE_TO_SERVER;
                        connections[i].cacheIndex = -1;
                    }
                }
            } else {
                dropConnectionWrapper(i, "CLIENT_MESSAGE:not good url", 0, connections,
                                      localConnectionsCount, threadId);
            }
        }
    }
}

void *work(void *param) {

    int threadId = *((int *) param);
    printf("START:id: %d\n", threadId);

    int localConnectionsCount = 0;
    struct pollfd fds[2 * MAX_CONNECTIONS];
    Connection connections[MAX_CONNECTIONS];

    while (true) {
        int newClientSocket = getNewClientSocketOrWait(&localConnectionsCount, threadId);

        if (newClientSocket != -1) {
            initNewConnection(&connections[localConnectionsCount - 1], newClientSocket);
            printf("salam");
        }

        updatePoll(fds, localConnectionsCount, connections);

        int polled = poll(fds, localConnectionsCount * 2, -1);
        if (polled < 0) {
            perror("poll error");
        } else if (polled == 0) {
            continue;
        }

        char buf[BUFFER_SIZE];
        for (int i = 0; i < localConnectionsCount; i++) {
            switch (connections[i].status) {
                case GETTING_REQUEST_FROM_CLIENT: {
                    handleGettingRequest(connections, fds, &localConnectionsCount, buf, threadId, i);
                    break;
                }
                case WRITE_TO_SERVER: {
                    if (fds[i * 2 + 1].revents & POLLOUT) {

                        if (send(connections[i].serverSocket, connections[i].buffer, connections[i].buffer_size, 0) <=
                            0) {
                            makeCacheInvalid(connections[i].cacheIndex);
                            dropConnectionWrapper(i, "WRITE_TO_SERVER:server err", 1,
                                                  connections, &localConnectionsCount, threadId);
                            break;
                        }
                        freeConnectionBuffer(&connections[i]);
                        connections[i].status = READ_FROM_SERVER_WRITE_CLIENT;
                    }
                    break;
                }
                case READ_FROM_SERVER_WRITE_CLIENT: {
                    if ((fds[i * 2].revents & POLLOUT && fds[i * 2 + 1].revents & POLLIN) ||
                        (connections[i].clientSocket == -1 && fds[i * 2 +
                                                                  1].revents & POLLIN)) {

                        ssize_t readCount = recv(connections[i].serverSocket, buf, BUFFER_SIZE, 0);
                        //printf("(%d) (%d)| %d\n", threadId, connections[i].id, readCount);

                        if (readCount < 0) {
                            printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT:recv fron server err\n", threadId,
                                   connections[i].id);
                            makeCacheInvalid(connections[i].cacheIndex);
                            dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:recv fron server err", 1,
                                                  connections, &localConnectionsCount, threadId);
                            break;
                        }

                        if (readCount == 0) {
                            if (connections[i].cacheIndex != -1) {
                                printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT:INVALID CACHE\n", threadId,
                                       connections[i].id);
                                makeCacheInvalid(connections[i].cacheIndex);
                            }
                            dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:server closed", 1, connections,
                                                  &localConnectionsCount, threadId);
                            break;
                        }

                        //-------------------------------------------------------------------------send client  = 0?
                        if (connections[i].clientSocket != -1) {
                            if (send(connections[i].clientSocket, buf, (size_t) readCount, 0) <= 0) {
                                printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT: CLIENT ERROR\n", threadId,
                                       connections[i].id);
                                connections[i].clientSocket = -1;
                            }
                        }


                        if (connections[i].cacheIndex == -1) {
                            printf("READ_FROM_SERVER_WRITE_CLIENT:i dont have cache");
                            break;
                        }
                        //---------------------------------------------------------------------------put to cache
                        if (DOWNLOADING == cache[connections[i].cacheIndex].status &&
                            cache[connections[i].cacheIndex].recvSize == 0) {

                            //printf("%d first time\n", connections[i].id);
                            char *dest = buf;
                            int body = getIndexOfBody(dest, readCount);
                            if (body == -1) { break; }

                            int statusCode = getStatusCodeAnswer(dest);
                            long contentLength = getContentLengthFromAnswer(dest);

                            if (statusCode != 200 || contentLength == -1) {
                                makeCacheInvalid(connections[i].cacheIndex);
                                dropConnectionWrapper(i, "DO NOT NEED TO BE CACHED", 1, connections,
                                                      &localConnectionsCount, threadId);
                                break;
                            }
                            cache[connections[i].cacheIndex].allSize = (size_t) (contentLength + body);
                        }

                        //printf("%d before realloc\n", connections[i].id);

                        char **tempPtr = (char **) realloc(cache[connections[i].cacheIndex].data,
                                                           (cache[connections[i].cacheIndex].numChunks + 1) *
                                                           sizeof(char *));
                        if (tempPtr == NULL) {
                            //free old data
                            printf("CACHE malloc failed\n");
                            makeCacheInvalid(connections[i].cacheIndex);
                            break;
                        }
                        cache[connections[i].cacheIndex].data = tempPtr;

                        int *tempPtr1 = (int *) realloc(cache[connections[i].cacheIndex].dataChunksSize,
                                                        (cache[connections[i].cacheIndex].numChunks
                                                         + 1) * sizeof(int));
                        if (tempPtr1 == NULL) {
                            //free old data
                            printf("CACHE malloc failed\n");
                            makeCacheInvalid(connections[i].cacheIndex);
                            break;
                        }
                        cache[connections[i].cacheIndex].dataChunksSize = tempPtr1;
                        cache[connections[i].cacheIndex].dataChunksSize[cache[connections[i].cacheIndex].numChunks] =
                                sizeof(char) * readCount;

                        //printf("SIZE = %d\n", cache[connections[i].cacheIndex].dataChunksSize[cache[connections[i].cacheIndex].numChunks]);

                        cache[connections[i].cacheIndex].data[cache[connections[i].cacheIndex].numChunks] = (char *) malloc(
                                sizeof(char) * readCount);

                        //printf("%d malloc size = %d\n", connections[i].id, sizeof(char)*readCount);

                        if (cache[connections[i].cacheIndex].data[cache[connections[i].cacheIndex].numChunks] == NULL) {
                            printf("CACHE malloc failed\n");
                            makeCacheInvalid(connections[i].cacheIndex);
                            break;
                        }

                        pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
                        //printf("%d 18\n", connections[i].id);
                        cache[connections[i].cacheIndex].recvSize += readCount;

                        memcpy(cache[connections[i].cacheIndex].data[cache[connections[i].cacheIndex].numChunks], buf,
                               sizeof(char) * readCount);

                        //printf("%d readCount = %d\n", connections[i].id, readCount);

                        pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);

                        pthread_mutex_lock(&cache[connections[i].cacheIndex].numChunksMutex);
                        cache[connections[i].cacheIndex].numChunks++;
                        //printf("%d numCH = %d\n", connections[i].id, cache[connections[i].cacheIndex].numChunks);
                        pthread_mutex_unlock(&cache[connections[i].cacheIndex].numChunksMutex);

                        pthread_cond_broadcast(&cache[connections[i].cacheIndex].numChunksCondVar);

                        if (cache[connections[i].cacheIndex].recvSize == cache[connections[i].cacheIndex].allSize) {
                            pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
                            cache[connections[i].cacheIndex].status = VALID;
                            pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);
                            dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:SUCCESS", 1, connections,
                                                  &localConnectionsCount, threadId);
                        }

                        break;
                    }
                    break;
                }
                case READ_FROM_CACHE_WRITE_CLIENT: {

                    if (fds[i * 2].revents & POLLOUT) {

                        //printf("%d 25\n", connections[i].id);
                        int localCacheStat;
                        size_t localNumChanks;

                        pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
                        localCacheStat = cache[connections[i].cacheIndex].status;
                        pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);

                        //printf("%d 26\n", connections[i].id);

                        if (localCacheStat == VALID || localCacheStat == DOWNLOADING) {

                            pthread_mutex_lock(&cache[connections[i].cacheIndex].numChunksMutex);

                            localNumChanks = cache[connections[i].cacheIndex].numChunks;

                            while (localCacheStat == DOWNLOADING && connections[i].numChunksWritten == localNumChanks &&
                                   localConnectionsCount ==
                                   1) {

                                pthread_cond_wait(&cache[connections[i].cacheIndex].numChunksCondVar,
                                                  &cache[connections[i].cacheIndex].numChunksMutex);
                                if (cache[connections[i].cacheIndex].status == INVALID) {
                                    dropConnectionWrapper(i,
                                                          "READ_FROM_CACHE_WRITE_CLIENT:smth happend with writer cache",
                                                          0, connections, &localConnectionsCount, threadId);
                                    break;
                                }
                                localNumChanks = cache[connections[i].cacheIndex].numChunks;
                            }

                            pthread_mutex_unlock(&cache[connections[i].cacheIndex].numChunksMutex);

                            for (size_t k = connections[i].numChunksWritten; k < localNumChanks; k++) {

                                ssize_t bytesWritten = send(connections[i].clientSocket,
                                                            cache[connections[i].cacheIndex].data[k],
                                                            cache[connections[i].cacheIndex].dataChunksSize[k], 0);

                                //printf("%d sizebuf=(%d)\n", connections[i].id, cache[connections[i].cacheIndex].dataChunksSize[k]);
                                if (bytesWritten <= 0) {
                                    dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:client err", 0, connections,
                                                          &localConnectionsCount, threadId);
                                    break;
                                }
                            }

                            connections[i].numChunksWritten = localNumChanks;

                            if (localCacheStat == VALID) {
                                dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:SUCCESS", 0, connections,
                                                      &localConnectionsCount, threadId);
                            }
                            break;
                        } else if (localCacheStat == INVALID) {
                            dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:smth happend with writer cache", 0,
                                                  connections, &localConnectionsCount, threadId);
                            break;
                        }

                    }//fds
                    break;
                }//read
            }//switch
        }//for
    }//while

    return NULL;
}//end

void checkArgs(int argcc, const char *argv[]) {
    checkCountArguments(argcc);
    poolSize = atoi(argv[1]);
    checkIfValidParsedInt(poolSize);
    int proxySocketPort = atoi(argv[2]);
    checkIfValidParsedInt(proxySocketPort);
}

int main(int argc, const char *argv[]) {

    checkArgs(argc, argv);
    int proxySocketPort = atoi(argv[2]);

    if (initMutex(&connectionsMutex) == -1) {
        printf("ERROR in initMUTEX");
        pthread_exit(NULL);
    }

    if (initCache(cache, MAX_CACHE_SIZE) == -1) {
        printf("ERROR in initCACHE");
        destroyCache(cache, MAX_CACHE_SIZE);
        pthread_exit(NULL);
    }

    int *threadsId = NULL;
    pthread_t *poolThreads = NULL;
    if (createThreadPool(poolSize, work, threadsId, poolThreads) == -1) {
        pthread_exit(NULL);
    }

    socketsQueue = createQueue();

    int proxySocket = getProxySocket(proxySocketPort);
    signal(SIGPIPE, SIG_IGN);

    while (true) {
        int newClientSocket = accept(proxySocket, (struct sockaddr *) NULL, NULL);

        if (newClientSocket != -1) {
            printf("ACCEPTED NEW CONNECTION\n");
            pthread_mutex_lock(&socketsQueue->queueMutex);
            putSocketInQueue(socketsQueue, newClientSocket);
            pthread_mutex_unlock(&socketsQueue->queueMutex);

            pthread_mutex_lock(&connectionsMutex);
            allConnectionsCount++;
            pthread_cond_broadcast(&socketsQueue->condVar);//?
            pthread_mutex_unlock(&connectionsMutex);
        }
    }

    close(proxySocket);
    free(threadsId);
    free(poolThreads);
    clearQueue(socketsQueue);
}