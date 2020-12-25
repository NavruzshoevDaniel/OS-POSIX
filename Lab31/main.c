#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include "services/queue/queueService.h"
#include "services/connection/connection.h"
#include "services/threadpool/threadPool.h"
#include "argschecker/argsChecker.h"
#include "services/concurrent/atomicInt.h"
#include "services/cache/cache.h"
#include "services/proxyhandlers/getRequest/getRequestHandler.h"
#include "services/proxyhandlers/writeToServer/writeToServerHandler.h"
#include "services/proxyhandlers/readFromServerWriteClientState/readFromServerWriteToClientHandler.h"

#define MAX_CONNECTIONS 100
#define MAX_CACHE_SIZE 3*1024
#define BUFFER_SIZE 16 * 1024
#define MAX_NUM_TRANSLATION_CONNECTIONS 100
#define MAX_CONNECTIONS_PER_THREAD allConnectionsCount / poolSize

//3 = CRLF EOF

Queue *socketsQueue;
static int allConnectionsCount = 0;
int poolSize;

CacheInfo cache[MAX_CACHE_SIZE];
pthread_mutex_t connectionsMutex;

void handleReadFromCacheWriteToClientState(Connection *connections,
                                           struct pollfd *fds,
                                           int *localConnectionsCount,
                                           int threadId,
                                           int i);

void handleReadFromServerWriteToClientStateWrapper(Connection *connections,
                                                   struct pollfd *fds,
                                                   int *localConnectionsCount,
                                                   char *buf,
                                                   int threadId,
                                                   int i);


int sendNewChunksToClient(Connection connection, size_t newSize);

void handleGettingRequestStateWrapper(Connection *connections,
                                      struct pollfd *fds,
                                      int *localConnectionsCount,
                                      char *buf,
                                      int threadId,
                                      int i);

void handleWriteToServerStateWrapper(Connection *connections,
                                     struct pollfd *fds,
                                     int *localConnectionsCount,
                                     int threadId,
                                     int i);

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
    pthread_mutex_lock(&socketsQueue->queueMutex);
    pthread_mutex_lock(&connectionsMutex);
    if (MAX_CONNECTIONS_PER_THREAD >= *localConnectionsCount && socketsQueue->size > 0) {

        newClientSocket = getSocketFromQueue(socketsQueue);
        if (newClientSocket != -1) {
            (*localConnectionsCount)++;
        }
    }
    pthread_mutex_unlock(&connectionsMutex);
    while (*localConnectionsCount == 0 && socketsQueue->size == 0) {
        pthread_cond_wait(&socketsQueue->condVar, &socketsQueue->queueMutex);
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
                           int needToCloseServer,
                           Connection *connections,
                           int *connectionsCount,
                           int threadId) {
    dropConnection(id, reason, needToCloseServer, connections, connectionsCount, threadId);
    atomicDecrement(&allConnectionsCount, &connectionsMutex);
}

_Noreturn void *work(void *param) {

    int threadId = *((int *) param);
    printf("START:id: %d\n", threadId);

    int localConnectionsCount = 0;
    struct pollfd fds[2 * MAX_CONNECTIONS];
    Connection connections[MAX_CONNECTIONS];

    while (true) {
        int newClientSocket = getNewClientSocketOrWait(&localConnectionsCount, threadId);

        if (newClientSocket != -1) {
            initNewConnection(&connections[localConnectionsCount - 1], newClientSocket);
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
                    handleGettingRequestStateWrapper(connections, fds, &localConnectionsCount, buf, threadId, i);
                    break;
                }
                case WRITE_TO_SERVER: {
                    handleWriteToServerStateWrapper(connections, fds, &localConnectionsCount, threadId, i);
                    break;
                }
                case READ_FROM_SERVER_WRITE_CLIENT: {
                    handleReadFromServerWriteToClientStateWrapper(connections, fds, &localConnectionsCount, buf,
                                                                  threadId, i);
                    break;
                }
                case READ_FROM_CACHE_WRITE_CLIENT: {
                    handleReadFromCacheWriteToClientState(connections, fds, &localConnectionsCount, threadId, i);
                    break;
                }
                case NOT_ACTIVE:
                    break;
            }
        }
    }
    return NULL;
}

void handleReadFromServerWriteToClientStateWrapper(Connection *connections,
                                                   struct pollfd *fds,
                                                   int *localConnectionsCount,
                                                   char *buf,
                                                   int threadId,
                                                   int i) {
    int result = handleReadFromServerWriteToClientState(&connections[i], fds[i * 2],
                                                        fds[i * 2 + 1],
                                                        cache, buf, BUFFER_SIZE, threadId);
    if (result == RECV_FROM_SERVER_EXCEPTION) {
        printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT:recv fron server err\n", threadId, connections[i].id);
        makeCacheInvalid(&cache[connections[i].cacheIndex]);
        dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:recv fron server err",
                              1, connections, localConnectionsCount, threadId);
    } else if (result == SERVER_CLOSED_EXCEPTION) {
        if (connections[i].cacheIndex != -1) {
            printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT:INVALID CACHE\n", threadId, connections[i].id);
            makeCacheInvalid(&cache[connections[i].cacheIndex]);
        }
        dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:server closed",
                              1, connections, localConnectionsCount, threadId);
    } else if (result == NOT_FREE_CACHE_EXCEPTION) {
        printf("READ_FROM_SERVER_WRITE_CLIENT:i dont have cache");
    } else if (result == STATUS_OR_CONTENT_LENGTH_EXCEPTION) {
        makeCacheInvalid(&cache[connections[i].cacheIndex]);
        dropConnectionWrapper(i, "DO NOT NEED TO BE CACHED",
                              1, connections, localConnectionsCount, threadId);
    } else if (result == END_READING_PROCCESS) {
        dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:SUCCESS",
                              1, connections, localConnectionsCount, threadId);
    } else if (result==PUT_CACHE_DATA_EXCEPTION){
        dropConnectionWrapper(i, "PUT_CACHE_DATA_EXCEPTION",
                              1, connections, localConnectionsCount, threadId);
    }
}

void handleWriteToServerStateWrapper(Connection *connections,
                                     struct pollfd *fds,
                                     int *localConnectionsCount,
                                     int threadId,
                                     int i) {
    if (SEND_TO_SERVER_EXCEPTION == handleWriteToServerState(&connections[i], fds[i * 2 + 1])) {
        makeCacheInvalid(&cache[connections[i].cacheIndex]);
        dropConnectionWrapper(i, "WRITE_TO_SERVER:server err", 1,
                              connections, localConnectionsCount, threadId);
    }
}

void handleGettingRequestStateWrapper(Connection *connections,
                                      struct pollfd *fds,
                                      int *localConnectionsCount,
                                      char *buf,
                                      int threadId,
                                      int i) {
    switch (handleGettingRequestState(&connections[i], buf, BUFFER_SIZE, threadId, fds[i * 2], cache, MAX_CACHE_SIZE)) {
        case DEAD_CLIENT_EXCEPTION: {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:dead client ", 0,
                                  connections, localConnectionsCount, threadId);
            break;
        }
        case RECV_CLIENT_EXCEPTION : {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:recv err", 0, connections,
                                  localConnectionsCount, threadId);
            break;
        }
        case ALLOCATING_BUFFER_MEMORY_EXCEPTION: {
            dropConnectionWrapper(i, "buffer error",
                                  0, connections, localConnectionsCount, threadId);
            break;
        }
        case NOT_GET_EXCEPTION: {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:not GET", 0,
                                  connections, localConnectionsCount, threadId);
            break;
        }
        case URL_EXCEPTION: {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:not good url", 0, connections,
                                  localConnectionsCount, threadId);
            break;
        }
        case RESOLVING_SOCKET_FROM_URL_EXCEPTION : {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:get server err", 0, connections, localConnectionsCount,
                                  threadId);
        }
    }
}

void handleReadFromCacheWriteToClientState(Connection *connections,
                                           struct pollfd *fds,
                                           int *localConnectionsCount,
                                           int threadId,
                                           int i) {
    if (fds[i * 2].revents & POLLOUT) {
        int localCacheStatus;
        size_t localNumChunks;

        pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
        localCacheStatus = cache[connections[i].cacheIndex].status;
        pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);

        if (localCacheStatus == VALID || localCacheStatus == DOWNLOADING) {

            pthread_mutex_lock(&cache[connections[i].cacheIndex].numChunksMutex);

            localNumChunks = cache[connections[i].cacheIndex].numChunks;

            while (localCacheStatus == DOWNLOADING && connections[i].numChunksWritten == localNumChunks &&
                   *localConnectionsCount == 1) {

                pthread_cond_wait(&cache[connections[i].cacheIndex].numChunksCondVar,
                                  &cache[connections[i].cacheIndex].numChunksMutex);
                if (cache[connections[i].cacheIndex].status == INVALID) {
                    dropConnectionWrapper(i,
                                          "READ_FROM_CACHE_WRITE_CLIENT:smth happend with writer cache",
                                          0, connections, localConnectionsCount, threadId);
                    pthread_mutex_unlock(&cache[connections[i].cacheIndex].numChunksMutex);
                    return;
                }
                localNumChunks = cache[connections[i].cacheIndex].numChunks;
            }

            pthread_mutex_unlock(&cache[connections[i].cacheIndex].numChunksMutex);

            if (sendNewChunksToClient(connections[i], localNumChunks) == -1) {
                dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:client err", 0, connections,
                                      localConnectionsCount, threadId);
                return;
            }
            connections[i].numChunksWritten = localNumChunks;

            if (localCacheStatus == VALID) {
                dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:SUCCESS", 0, connections,
                                      localConnectionsCount, threadId);
            }
            return;
        } else if (localCacheStatus == INVALID) {
            dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:smth happend with writer cache", 0,
                                  connections, localConnectionsCount, threadId);
            return;
        }
    }
}

int sendNewChunksToClient(Connection connection, size_t newSize) {

    for (size_t k = connection.numChunksWritten; k < newSize; k++) {
        ssize_t bytesWritten = send(connection.clientSocket,
                                    cache[connection.cacheIndex].data[k],
                                    cache[connection.cacheIndex].dataChunksSize[k], 0);
        if (bytesWritten <= 0) {
            return -1;
        }
    }
    return 0;
}

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

    socketsQueue = createQueue();

    int proxySocket = getProxySocket(proxySocketPort);
    signal(SIGPIPE, SIG_IGN);

    if (createThreadPool(poolSize, work, threadsId, poolThreads) == -1) {
        pthread_exit(NULL);
    }

    while (true) {
        int newClientSocket = accept(proxySocket, (struct sockaddr *) NULL, NULL);

        if (newClientSocket != -1) {
            printf("ACCEPTED NEW CONNECTION\n");

            pthread_mutex_lock(&socketsQueue->queueMutex);
            putSocketInQueue(socketsQueue, newClientSocket);
            pthread_mutex_unlock(&socketsQueue->queueMutex);

            atomicIncrement(&allConnectionsCount, &connectionsMutex);
            pthread_cond_signal(&socketsQueue->condVar);
        }
    }
    pthread_exit(NULL);
}
