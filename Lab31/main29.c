#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>
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
#include "services/http/httpService.h"

#define MAX_CONNECTIONS 100
#define MAX_CACHE_SIZE 3*1024
#define BUFFER_SIZE 16 * 1024
#define MAX_NUM_TRANSLATION_CONNECTIONS 100

int isRun=1;

//3 = CRLF EOF

int proxySocketPort;

CacheInfo cache[MAX_CACHE_SIZE];

void handleWriteToServerState(Connection *connections,
                              struct pollfd *fds,
                              int *localConnectionsCount,
                              int threadId,
                              int i);

void handleGetMethod(char *url, Connection *connection, int i, int *localConnectionsCount, int threadId);

void handleReadFromServerWriteToClientState(Connection *connections,
                                            struct pollfd *fds,
                                            int *localConnectionsCount,
                                            char *buf,
                                            int threadId,
                                            int i);

bool isFirstCacheChunk(const CacheInfo *cache);

void handleReadFromCacheWriteToClientState();

int sendNewChunksToClient(Connection connection, size_t newSize);

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

void handleNotGetMethod(struct Connection *connection) {
    char wrong[] = "HTTP: 405\r\nAllow: GET\r\n";
    write(connection->clientSocket, wrong, 23);
}

void handleNotResolvingUrl(struct Connection *connection) {
    char errorstr[] = "HTTP: 523\r\n";
    write(connection->clientSocket, errorstr, 11);
}

void updatePoll(struct pollfd *fds, int localCount, int proxySocket, Connection *connections) {
    fds[localCount * 2].fd = proxySocket;
    fds[localCount * 2].events = POLLIN;
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


void dropConnectionWrapper(int id,
                           const char *reason,
                           int needToCloseServer,
                           Connection *connections,
                           int *connectionsCount,
                           int threadId) {
    dropConnection(id, reason, needToCloseServer, connections, connectionsCount, threadId);
}

void handleGettingRequestState(Connection *connections,
                               struct pollfd *fds,
                               int *localConnectionsCount,
                               char *buf,
                               int threadId,
                               int i) {
    if (fds[i*2].revents & POLLHUP) {
        dropConnectionWrapper(i, "CLIENT_MESSAGE:dead client ", 0,
                              connections, localConnectionsCount, threadId);
    } else if (fds[i*2].revents & POLLIN) {
        ssize_t readCount = recv(connections[i].clientSocket, buf, BUFFER_SIZE, 0);

        if (readCount <= 0) {
            dropConnectionWrapper(i, "CLIENT_MESSAGE:recv err", 0, connections,
                                  localConnectionsCount, threadId);
            return;
        }
        int bufferErr;
        if (isConnectionBufferEmpty(&connections[i])) {
            bufferErr = allocateConnectionBufferMemory(&connections[i], readCount);
        } else {
            printf("REALLOCATE CACHE");
            //TODO::this is really need?
            bufferErr = reallocateConnectionBufferMemory(&connections[i], readCount);
        }

        if (bufferErr == -1) {
            dropConnectionWrapper(i, "buffer error",
                                  0, connections, localConnectionsCount, threadId);
            return;
        }

        memcpy(connections[i].buffer, buf, (size_t) readCount);

        if (connections[i].buffer_size > 3) {
            if (!isMethodGet(connections[i].buffer)) {
                handleNotGetMethod(&connections[i]);
                dropConnectionWrapper(i, "CLIENT_MESSAGE:not GET", 0,
                                      connections, localConnectionsCount, threadId);

            } else {
                char *url = getUrlFromData(connections[i].buffer);
                if (url != NULL) {
                    handleGetMethod(url, connections, i, localConnectionsCount, threadId);
                } else {
                    dropConnectionWrapper(i, "CLIENT_MESSAGE:not good url", 0, connections,
                                          localConnectionsCount, threadId);
                }
            }
        }


    }
}

void handleGetMethod(char *url, Connection *connections, int i, int *localConnectionsCount, int threadId) {
    int urlInCacheResult = searchUrlInCache(url, cache, MAX_CACHE_SIZE);
    if (urlInCacheResult >= 0) {
        setReadFromCacheState(&connections[i], urlInCacheResult);
    } else {
        int freeCacheIndex = searchFreeCacheAndSetDownloadingState(url, cache, MAX_CACHE_SIZE, threadId);
        if ((-1 != freeCacheIndex) ||
            (-1 != (freeCacheIndex =
                            searchNotUsingCacheAndSetDownloadingState(url, cache, MAX_CACHE_SIZE, threadId)))) {

            setWriteToServerState(&connections[i], freeCacheIndex);
            connections[i].serverSocket = getServerSocketBy(url);
            free(connections[i].buffer);

            connections[i].buffer = createGet(url, &connections[i].buffer_size);

            if (connections[i].serverSocket == -1) {
                handleNotResolvingUrl(&connections[i]);
                dropConnectionWrapper(i, "CLIENT_MESSAGE:get server err", 0,
                                      connections, localConnectionsCount, threadId);
                free(url);
                return;
            }
        } else {
            setWriteToServerState(&connections[i], -1);
        }
    }
}
int proxySocket;

_Noreturn void *work(void *param) {

    int threadId = *((int *) param);
    printf("START:id: %d\n", threadId);
     proxySocket = getProxySocket(proxySocketPort);
    int localConnectionsCount = 0;
    struct pollfd fds[2 * MAX_CONNECTIONS];
    Connection connections[MAX_CONNECTIONS];

    while (isRun) {
        updatePoll(fds, localConnectionsCount, proxySocket, connections);

        int polled = poll(fds, localConnectionsCount * 2 + 1, -1);
        if (polled < 0) {
            perror("poll error");
        } else if (polled == 0) {
            continue;
        }

        char buf[BUFFER_SIZE];
        for (int i = 0; i < localConnectionsCount; i++) {

            switch (connections[i].status) {
                case GETTING_REQUEST_FROM_CLIENT: {
                    //printf("connection %d handleGettingRequestState\n", i);
                    handleGettingRequestState(connections, fds, &localConnectionsCount, buf, threadId, i);
                    break;
                }
                case WRITE_TO_SERVER: {
                    //printf("connection %d handleWriteToServerState\n", i);
                    handleWriteToServerState(connections, fds, &localConnectionsCount, threadId, i);
                    break;
                }
                case READ_FROM_SERVER_WRITE_CLIENT: {
                    //printf("connection %d handleReadFromServerWriteToClientState\n", i);
                    handleReadFromServerWriteToClientState(connections, fds, &localConnectionsCount, buf, threadId, i);
                    break;
                }
                case READ_FROM_CACHE_WRITE_CLIENT: {
                    //printf("connection %d handleReadFromCacheWriteToClientState\n", i);
                    handleReadFromCacheWriteToClientState(connections, fds, &localConnectionsCount, threadId, i);
                    break;
                }
                case NOT_ACTIVE:
                    break;
            }
        }

        if (fds[localConnectionsCount * 2].revents & POLLIN) {
            int newClientSocket = accept(proxySocket, (struct sockaddr *) NULL, NULL);
            printf("ACCEPTED NEW CONNECTION FD: %d\n", newClientSocket);
            if (localConnectionsCount >= MAX_CONNECTIONS) {
                close(newClientSocket);
                continue;
            }
            if (newClientSocket != -1) {
                initNewConnection(&connections[localConnectionsCount], newClientSocket);
                localConnectionsCount++;
            }
        }

    }
    return NULL;
}

void handleReadFromCacheWriteToClientState(Connection *connections,
                                           struct pollfd *fds,
                                           int *localConnectionsCount,
                                           int threadId,
                                           int i) {
    if (fds[i * 2].revents & POLLOUT) {
        //printf("handleReadFromCacheWriteToClientState by i=%d\n",i);
        int localCacheStatus;
        size_t totalNumChunks;

        localCacheStatus = cache[connections[i].cacheIndex].status;

        if (localCacheStatus == VALID || localCacheStatus == DOWNLOADING) {

            totalNumChunks = cache[connections[i].cacheIndex].numChunks;
            size_t nowChunksWritten = connections[i].numChunksWritten;
            if (nowChunksWritten < totalNumChunks) {
                //printf("%s\n\n\n", cache[connections[i].cacheIndex].data[nowChunksWritten]);
                ssize_t bytesWritten = send(connections[i].clientSocket, cache[connections[i].cacheIndex].data[nowChunksWritten],
                                            cache[connections[i].cacheIndex].dataChunksSize[nowChunksWritten], 0);
                //printf("%d\n", bytesWritten);
                if (bytesWritten < 0) {
                    printf("error: %s\n clientSocket: %d\n", strerror(errno), connections[i].clientSocket);
                    printf( "%zu\n",connections[i].numChunksWritten);
                    printf( "total %zu\n",cache[connections[i].cacheIndex].numChunks);
                    dropConnectionWrapper(i, "READ_FROM_CACHE_WRITE_CLIENT:client err", 0, connections,
                                          localConnectionsCount, threadId);
                    return;
                }
                connections[i].numChunksWritten++;
            }


            if (localCacheStatus == VALID && connections[i].numChunksWritten == totalNumChunks) {
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

void handleReadFromServerWriteToClientState(Connection *connections,
                                            struct pollfd *fds,
                                            int *localConnectionsCount,
                                            char *buf,
                                            int threadId,
                                            int i) {
    if ((fds[i * 2].revents & POLLOUT && fds[i * 2 + 1].revents & POLLIN) ||
        (connections[i].clientSocket == -1 && fds[i * 2 + 1].revents & POLLIN)) {

        ssize_t readCount = recv(connections[i].serverSocket, buf, BUFFER_SIZE, 0);
        //printf("(%d) (%d)| %d\n", threadId, connections[i].id, readCount);
        //printf("recv()\n");
        if (readCount < 0) {
            printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT:recv fron server err\n", threadId,
                   connections[i].id);
            makeCacheInvalid(&cache[connections[i].cacheIndex]);
            dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:recv fron server err", 1,
                                  connections, localConnectionsCount, threadId);
            return;
        }

        if (readCount == 0) {
            if (connections[i].cacheIndex != -1) {
                printf("(%d) (%d)| READ_FROM_SERVER_WRITE_CLIENT:INVALID CACHE\n", threadId,
                       connections[i].id);
                makeCacheInvalid(&cache[connections[i].cacheIndex]);
            }
            dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:server closed", 1, connections,
                                  localConnectionsCount, threadId);
            return;
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
            return;
        }

        if (isFirstCacheChunk(&cache[connections[i].cacheIndex])) {
            //printf("%d first time\n", connections[i].id);
            char *dest = buf;
            int body = getIndexOfBody(dest, readCount);
            if (body == -1) { return; }

            int statusCode = getStatusCodeAnswer(dest);
            long contentLength = getContentLengthFromAnswer(dest);

            if (statusCode != 200 || contentLength == -1) {
                makeCacheInvalid(&cache[connections[i].cacheIndex]);
                dropConnectionWrapper(i, "DO NOT NEED TO BE CACHED", 1, connections,
                                      localConnectionsCount, threadId);
                return;
            }
            cache[connections[i].cacheIndex].allSize = (size_t) (contentLength + body);
        }
        //printf("if is\n");

        //printf("%d before realloc\n", connections[i].id);
        putDataToCache(&cache[connections[i].cacheIndex], buf, readCount);

        if (cache[connections[i].cacheIndex].recvSize == cache[connections[i].cacheIndex].allSize) {
            cache[connections[i].cacheIndex].status = VALID;
            dropConnectionWrapper(i, "READ_FROM_SERVER_WRITE_CLIENT:SUCCESS", 1, connections,
                                  localConnectionsCount, threadId);
        }
    }
}

bool isFirstCacheChunk(const CacheInfo *cache) {
    return DOWNLOADING == cache->status && cache->recvSize == 0;
}

void handleWriteToServerState(Connection *connections,
                              struct pollfd *fds,
                              int *localConnectionsCount,
                              int threadId,
                              int i) {
    if (fds[i * 2 + 1].revents & POLLOUT) {
        if (send(connections[i].serverSocket, connections[i].buffer, connections[i].buffer_size, 0) <=
            0) {
            makeCacheInvalid(&cache[connections[i].cacheIndex]);
            dropConnectionWrapper(i, "WRITE_TO_SERVER:server err", 1,
                                  connections, localConnectionsCount, threadId);
            return;
        }
        setReadFromServerWriteToClientState(&connections[i]);
    }
}
//end

void checkArgs(int argcc, const char *argv[]) {
    int proxySocketPort = atoi(argv[1]);
    checkIfValidParsedInt(proxySocketPort);
}

void exitHandler(){
    printf("closing");
   // close(proxySocket);
}

void sigHandler(int sig){
    printf("sigHandler");
    isRun=0;
    close(proxySocket);
    pthread_exit(NULL);
}
int main(int argc, const char *argv[]) {
    atexit(exitHandler);
    signal(SIGINT,sigHandler);
    signal(SIGTERM,sigHandler);
    checkArgs(argc, argv);
    proxySocketPort = atoi(argv[1]);

    if (initCache(cache, MAX_CACHE_SIZE) == -1) {
        printf("ERROR in initCACHE");
        destroyCache(cache, MAX_CACHE_SIZE);
        pthread_exit(NULL);
    }

    signal(SIGPIPE, SIG_IGN);

    int number = 0;
    work(&number);
    close(proxySocketPort);
    pthread_exit(NULL);
}
