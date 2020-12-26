#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include "services/queue/queueService.h"
#include "services/connection/connection.h"
#include "services/threadpool/threadPool.h"
#include "argschecker/argsChecker.h"
#include "services/concurrent/atomicInt.h"
#include "services/cache/cache.h"
#include "services/proxyhandlers/getRequest/getRequestHandler.h"
#include "services/proxyhandlers/writeToServer/writeToServerHandler.h"
#include "services/proxyhandlers/readFromServerWriteClient/readFromServerWriteToClientHandler.h"
#include "services/proxyhandlers/readFromCacheWriteToClient/readFromCacheWriteToClientState.h"
#include "services/net/serverSockerService.h"
#include "config.h"

#define MAX_CONNECTIONS 100
#define MAX_CACHE_SIZE 3*1024
#define BUFFER_SIZE 16 * 1024
#define MAX_NUM_TRANSLATION_CONNECTIONS 100
#define MAX_CONNECTIONS_PER_THREAD allConnectionsCount / poolSize

//3 = CRLF EOF

Queue *socketsQueue;
static int allConnectionsCount = 0;
int poolSize;
int isRun = 1;
bool sigCaptured = false;

CacheInfo cache[MAX_CACHE_SIZE];
pthread_mutex_t connectionsMutex;
int proxySocket;

void handleReadFromCacheWriteToClientStateWrapper(Connection *connections,
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
    if (MAX_CONNECTIONS_PER_THREAD >= *localConnectionsCount && !isEmpty(socketsQueue) && isRun == 1) {

        newClientSocket = getSocketFromQueue(socketsQueue);
        if (newClientSocket != -1) {
            (*localConnectionsCount)++;
        }
    }
    pthread_mutex_unlock(&connectionsMutex);
    while (*localConnectionsCount == 0 && isEmpty(socketsQueue) && isRun == 1) {
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

void *work(void *param) {

    int threadId = *((int *) param);
    printf("START:id: %d\n", threadId);

    int localConnectionsCount = 0;

#ifdef _MULTITHREAD
    struct pollfd fds[2 * MAX_CONNECTIONS];
#else
    struct pollfd fds[2 * MAX_CONNECTIONS+1];
#endif

    Connection connections[MAX_CONNECTIONS];

    while (isRun == 1) {
#ifdef _MULTITHREAD
        int newClientSocket = getNewClientSocketOrWait(&localConnectionsCount, threadId);
        //printf("getNewClientSocketOrWait()\n");
        if (newClientSocket != -1) {
            initNewConnection(&connections[localConnectionsCount - 1], newClientSocket);
        }
#else
        int newClientSocket =-1;
        fds[localConnectionsCount * 2].fd = proxySocket;
        fds[localConnectionsCount * 2].events = POLLIN;
#endif
        if (isRun == 1) {
            updatePoll(fds, localConnectionsCount, connections);
        } else { break; }
#ifdef _MULTITHREAD
        int polled = 0;
        if (isRun == 1) {
            polled = poll(fds, localConnectionsCount * 2, -1);
        } else { break; }
#else
        int polled = poll(fds, localConnectionsCount * 2+1, -1);
#endif

        if (polled < 0) {
            perror("poll error");
        } else if (polled == 0) {
            continue;
        }
#ifdef _MULTITHREAD
#else
        if (0 != (fds[localConnectionsCount*2].revents & POLLIN)) {

            if (localConnectionsCount < MAX_CONNECTIONS) {
                int newClientSocket = accept(proxySocket, (struct sockaddr *) NULL, NULL);
                if (newClientSocket != -1) {
                    initNewConnection(&connections[localConnectionsCount], newClientSocket);
                    printf("ACCEPTED NEW CONNECTION %d\n", localConnectionsCount);
                    localConnectionsCount++;
                }
            }

        }

#endif

        char buf[BUFFER_SIZE];
        for (int i = 0; i < localConnectionsCount; i++) {
            switch (connections[i].status) {
                case GETTING_REQUEST_FROM_CLIENT: {
                    printf("GETTING_REQUEST_FROM_CLIENT");
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
                    handleReadFromCacheWriteToClientStateWrapper(connections, fds, &localConnectionsCount, threadId, i);
                    break;
                }
                case NOT_ACTIVE:
                    break;
            }
        }
    }
    printf("while skipped. localConnectionsCount = %d\n", localConnectionsCount);
    for (int i = 0; i < localConnectionsCount; ++i) {
        if (connections[i].clientSocket != -1) {
            close(connections[i].clientSocket);
        }
        if (connections[i].serverSocket != -1) {
            close(connections[i].serverSocket);
        }
    }
    printf("End thread-%d\n", threadId);
    pthread_exit(NULL);
}

void handleReadFromCacheWriteToClientStateWrapper(Connection *connections,
                                                  struct pollfd *fds,
                                                  int *localConnectionsCount,
                                                  int threadId,
                                                  int i) {
    switch (handleReadFromCacheWriteToClientState(&connections[i], fds[i * 2], cache, localConnectionsCount)) {
        case WRITER_CACHE_INVALID_EXCEPTION: {
            dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:smth happend with writer cache",
                                  0, connections, localConnectionsCount, threadId);
            break;
        }
        case SEND_TO_CLIENT_EXCEPTION: {
            dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:client err",
                                  0, connections, localConnectionsCount, threadId);
            break;
        }
        case SUCCESS_WITH_END: {
            dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:SUCCESS",
                                  0, connections, localConnectionsCount, threadId);
            break;
        }
        case CACHE_INVALID_EXCEPTION: {
            dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:smth happend with writer cache",
                                  0, connections, localConnectionsCount, threadId);
            break;
        }
    }
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
    } else if (result == PUT_CACHE_DATA_EXCEPTION) {
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


void checkArgs(int argcc, const char *argv[]) {
#ifdef _MULTITHREAD
    checkCountArguments(argcc);
    poolSize = atoi(argv[1]);
    checkIfValidParsedInt(poolSize);
#endif
    int proxySocketPort = atoi(argv[2]);
    checkIfValidParsedInt(proxySocketPort);
}

void signalHandler(int sig) {
    if (sig == SIGTERM) {
        write(0, "SIGTERM\n", 8);
        //close(proxySocket);
    }
    if (sig == SIGINT) {
        write(0, "SIGINT\n", 7);
    }
    isRun = 0;
    sigCaptured = true;
    pthread_cond_broadcast(&socketsQueue->condVar);
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

    proxySocket = getProxySocket(proxySocketPort, MAX_NUM_TRANSLATION_CONNECTIONS);
    if (proxySocket < 0) { exit(NULL); }
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, SIG_IGN);


    struct pollfd proxyFds[1];
    proxyFds[0].fd = proxySocket;
    proxyFds[0].events = POLLIN;

#ifdef _MULTITHREAD
    socketsQueue = createQueue();
    if (createThreadPool(poolSize, work, threadsId, &poolThreads) == -1) {
        pthread_exit(NULL);
    }
    if (poolThreads == NULL) {printf("pidor suka\n");}
    while (true) {

        int newClientSocket = acceptPollWrapper(proxyFds, proxySocket, 1);
        printf("acceptPoolWrapper()\n");
        if (sigCaptured) {
            printf("if ili ne if\n");
            for (int i = 0; i < 5; i++) {
                printf("joining...");
                //printf("cancel = %d\n", pthread_cancel(poolThreads[i]));
                int rv = pthread_join(poolThreads[i], NULL);
                if (rv != 0) {
                    printf("some problems\n");
                }
                printf("done!\n");
            }
            break;
        }

        write(0, "?\n", 2);

        if (newClientSocket != -1) {
            printf("ACCEPTED NEW CONNECTION\n");

            pthread_mutex_lock(&socketsQueue->queueMutex);
            putSocketInQueue(socketsQueue, newClientSocket);
            pthread_mutex_unlock(&socketsQueue->queueMutex);

            atomicIncrement(&allConnectionsCount, &connectionsMutex);
            pthread_cond_signal(&socketsQueue->condVar);
        } else { break; }

    }

#else
    int param=0;
    work(&param);
#endif

    close(proxySocket);

    printf("close socket");
    pthread_exit(NULL);
}
