#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
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

#define MAX_CONNECTIONS 100
#define MAX_CACHE_SIZE 3*1024
#define BUFFER_SIZE 16 * 1024
#define MAX_NUM_TRANSLATION_CONNECTIONS 100

//3 = CRLF EOF

//----------------------------------------------TYPES
enum cacheStatus {
    DOWNLOADING,
    VALID,
    INVALID
};

struct cacheInfo {
    size_t allSize;
    size_t recvSize;
    pthread_mutex_t mutex;
    size_t readers;

    char **data;
    int *dataChunksSize;
    size_t numChunks;
    pthread_cond_t numChunksCondVar;
    pthread_mutex_t numChunksMutex;

    int writerId;
    char *url;
    enum cacheStatus status;
};

Queue *socketsQueue;
static int allConnectionsCount = 0;
int poolSize;

struct cacheInfo cache[MAX_CACHE_SIZE];

pthread_mutex_t connectionsMutex;

//----------------------------------------------------------------------------------PARSE
char *createGet(char *url, size_t *len) {

    //printf("URL=%s\n",url);
    //protocol
    char *afterProtocol = strstr(url, "://");
    if (NULL == afterProtocol) {
        fprintf(stderr, "Incorrect input.\n");
    }

    //host
    int hostLength;

    char *afterHost = strchr(afterProtocol + 3, '/');
    if (NULL == afterHost) {
        hostLength = strlen(afterProtocol + 3);
    } else {
        hostLength = afterHost - (afterProtocol + 3);
    }

    char hostName[hostLength + 1];
    strncpy(hostName, afterProtocol + 3, hostLength);
    hostName[hostLength] = 0;
    //---------------------------------------------Send request

    char format[] = "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n\0";
    //printf("format size = %d",sizeof(format));

    char *buffer;
    if (NULL == afterHost) {
        buffer = (char *) malloc(sizeof(char) * (27 + 1 + sizeof(hostName)));
        *len = 27 + 1 + sizeof(hostName);
        sprintf(buffer, format, "/", hostName);
    } else {
        buffer = (char *) malloc(sizeof(char) * (27 + strlen(afterHost) + sizeof(hostName)));
        *len = 27 + strlen(afterHost) + sizeof(hostName);
        //printf("LEN=%d",*len);
        sprintf(buffer, format, afterHost, hostName);
    }

    //printf("newGetMethod = (%s)\n", buffer);
    return buffer;
}


int isMethodGet(char *httpData) {
    return httpData[0] == 'G' &&
           httpData[1] == 'E' &&
           httpData[2] == 'T' &&
           httpData[3] == ' ';
}

char *getUrlFromData(char *httpData) {

    char *smth = httpData + 4;
    char *endSmth = strchr(smth, ' ');

    char *result = (char *) malloc(sizeof(char) * (endSmth - smth + 1));
    if (NULL == result) {
        printf("ERROR WHILE MALLOC getUrlFromData");
    }

    memcpy(result, smth, (size_t) (endSmth - smth));
    result[endSmth - smth] = '\0';

    //printf("URL:%s\n", result);
    char *KASPSHIT = strstr(httpData, "kis.v2.scr.kaspersky");
    if (KASPSHIT != NULL) {
        return NULL;
    }
    return result;
}

char *getHostFromUrl(char *sH) {

    char *startHost = sH + 7;
    char *endHost = strchr(startHost, '/');

    char *result = (char *) malloc(sizeof(char) * (endHost - startHost + 1));
    if (NULL == result) {
        printf("ERROR WHILE MALLOC getHostFromUrl");
        //return 1;

    }

    memcpy(result, startHost, (size_t) (endHost - startHost));
    result[endHost - startHost] = '\0';

    //printf("Host:%s\n", result);
    return result;
}

int getStatusCodeAnswer(char *httpData) {
    char *afterHTTP = httpData + 9;// ignore "HTTP/1.1 "

    char *StrAfterCode;
    int statusCode = (int) strtol(afterHTTP, &StrAfterCode, 10);
    if (StrAfterCode == afterHTTP || statusCode <= 0) {
        return -1;
    }
    return statusCode;
}

long getContentLengthFromAnswer(char *httpData) {

    char header[] = "Content-Length:";

    char *startLength = strstr(httpData, header);

    if (startLength == NULL) {
        return -1;
    }

    startLength += strlen(header) + 1;
    char *StrAfterLength;
    long contentLength = strtol(startLength, &StrAfterLength, 10);
    if (StrAfterLength == startLength || contentLength <= 0) {
        return -1;
    }
    //printf("ContLength = %d\n", contentLength);
    return contentLength;
}

int needResend(int statusCode) {
    if (statusCode == 204) {//No Content
        return 0;
    }

    if (statusCode == 304) {//Not Modified
        return 0;
    }

    if (statusCode >= 100 && statusCode < 200) {
        return 0;
    }

    return 1;
}

void editRequestToSendToServer(struct Connection *attrSocket) {///////////////////////////////////////////////////////

    char *protocol_1_1 = strstr(attrSocket->buffer, "HTTP/1.1");/////////////->HTTP/1.0

    if (protocol_1_1 != NULL) {
        protocol_1_1[7] = '0';
    }

}

void cutBody(char *buff, size_t *len, size_t newLength) {
    buff = (char *) realloc(buff, newLength);
    if (buff == NULL) {
        fprintf(stderr, "realloc failed at method cutBody()\n");
        exit(EXIT_FAILURE);
    }
    buff[newLength - 1] = '\0';
    *len = newLength;
}

int getIndexOfBody(char *buff, size_t len) {
    for (size_t j = 0; j < len - 3; ++j) {
        if (buff[j] == '\r' && buff[j + 1] == '\n' &&
            buff[j + 2] == '\r' && buff[j + 3] == '\n') {

            return (int) j + 4;
        }
    }

    return -1;
}

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

void wrongMethod(struct Connection *attrSocket) {
    char wrong[] = "HTTP: 405\r\nAllow: GET\r\n";
    write(attrSocket->clientSocket, wrong, 23);
}

void cannotResolve(struct Connection *attrSocket) {
    char errorstr[] = "HTTP: 523\r\n";
    write(attrSocket->clientSocket, errorstr, 11);
}
//----------------------------------------------------------------------------------CACHE

void freeData(char **data, size_t numChunks) {

    for (size_t i = 0; i < numChunks; i++) {
        free(data[i]);
    }
    printf("free data\n");
    free(data);
}

int searchCache(char *url, Connection *connection, int threadId) {

    //----------------------------------------------try find url in cache ( 0 1)
    for (int j = 0; j < MAX_CACHE_SIZE; j++) {

        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].url != NULL) {
            //printf("(%s)\n(%s)\n", cache[j].url,url);
            //printf("%d\n", strcmp(cache[j].url, url));
        }

        if (cache[j].url != NULL && strcmp(cache[j].url, url) == 0) {

            if (cache[j].status == VALID || cache[j].status == DOWNLOADING) {
                //printf("valid download\n");
                cache[j].readers++;
                freeConnectionBuffer(connection);
                connection->buffer_size = 0;
                connection->cacheIndex = j;
                connection->numChunksWritten = 0;
                connection->status = READ_FROM_CACHE_WRITE_CLIENT;
                pthread_mutex_unlock(&cache[j].mutex);
                return 0;
            }

            pthread_mutex_unlock(&cache[j].mutex);
        } else { pthread_mutex_unlock(&cache[j].mutex); }
    }

    //----------------------------------------------no url in cache, try find free cache (2)

    for (int j = 1; j < MAX_CACHE_SIZE; j++) {

        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].url == NULL) {
            //printf("(%d)SEARCH_CACHE: found free cache id=%d\n", threadId, j);
            cache[j].readers = 1;
            cache[j].status = DOWNLOADING;
            connection->cacheIndex = j;
            cache[j].writerId = threadId;
            cache[j].data = NULL;
            cache[j].dataChunksSize = NULL;
            cache[j].numChunks = 0;
            cache[j].allSize = 0;
            cache[j].recvSize = 0;
            connection->status = WRITE_TO_SERVER;

            cache[j].url = (char *) malloc(sizeof(char) * strlen(url) + 1);
            memcpy(cache[j].url, url, sizeof(char) * strlen(url) + 1);

            pthread_mutex_unlock(&cache[j].mutex);
            return 1;
        } else { pthread_mutex_unlock(&cache[j].mutex); }
    }

    //----------------------------------------------no url in cache, try find not using cache (2)

    for (int j = 0; j < MAX_CACHE_SIZE; j++) {

        pthread_mutex_lock(&cache[j].mutex);

        if (cache[j].readers == 0 || cache[j].status == INVALID) {
            //printf("(%d)SEARCH_CACHE: found not using cache id=%d\n", threadId, j);
            cache[j].readers = 1;
            cache[j].status = DOWNLOADING;
            connection->cacheIndex = j;
            cache[j].writerId = threadId;
            cache[j].numChunks = 0;
            cache[j].allSize = 0;
            cache[j].recvSize = 0;

            freeData(cache[j].data, cache[j].numChunks);
            free(cache[j].dataChunksSize);
            cache[j].data = NULL;
            cache[j].dataChunksSize = NULL;

            connection->status = WRITE_TO_SERVER;

            free(cache[j].url);
            cache[j].url = (char *) malloc(sizeof(char) * sizeof(url));
            memcpy(cache[j].url, url, sizeof(char) * sizeof(url));

            pthread_mutex_unlock(&cache[j].mutex);
            return 2;
        } else { pthread_mutex_unlock(&cache[j].mutex); }
    }

    //----------------------------------------------no cache (3)

    connection->status = WRITE_TO_SERVER;
    connection->cacheIndex = -1;
    //printf("no cache\n");
    return 3;

}

void makeCacheInvalid(int id) {
    pthread_mutex_lock(&cache[id].mutex);
    cache[id].status = INVALID;
    cache[id].writerId = -1;
    pthread_mutex_unlock(&cache[id].mutex);
    pthread_cond_broadcast(&cache[id].numChunksCondVar);
}

int initCache() {

    bool erMS, erCVC, erMC;
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {

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

void destroyCache() {

    for (int i = 0; i < MAX_CACHE_SIZE; i++) {

        pthread_mutex_destroy(&cache[i].mutex);

        freeData(cache[i].data, cache[i].numChunks);
        pthread_cond_destroy(&cache[i].numChunksCondVar);
        pthread_mutex_destroy(&cache[i].numChunksMutex);

        free(cache[i].url);
    }
    printf("destroy cache\n");
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
        }
    }
}

char *allocateMemory(char *buf, size_t *length, size_t additionalCount) {
    if (*length == 0) {
        *length = additionalCount;
        buf = (char *) malloc(*length * sizeof(char));

        if (NULL == buf) {
            perror("ERROR WHILE MALLOC allocateMemory");
        }
    } else {
        *length += (size_t) additionalCount;
        buf = (char *) realloc(buf, *length);
    }
    if (buf == NULL) {
        printf("allocation memory failed in method allocateMemory() %s\n", strerror(errno));
    }
    //printf("END malloc %d\n\n", *length);
    return buf;
}

void initNewConnection(Connection *connection, const int newClientSocket) {
    (*connection).clientSocket = newClientSocket;
    (*connection).buffer_size = 0;
    (*connection).buffer = NULL;
    (*connection).cacheIndex = -1;
    (*connection).serverSocket = -1;
    (*connection).status = GETTING_REQUEST_FROM_CLIENT;
    (*connection).id = rand() % 9000 + 1000;
}

int getNewClientSocketOrWait(int *localConnectionsCount) {
    int newClientSocket = -1;
    pthread_mutex_lock(&socketsQueue->queueMutex);
    pthread_mutex_lock(&connectionsMutex);
    if (allConnectionsCount / poolSize >= *localConnectionsCount && socketsQueue->size > 0) {

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

void *work(void *param) {
    //---------------------------------------------prepare
    int threadId = *((int *) param);
    printf("START:id: %d\n", threadId);

    int localConnectionsCount = 0;
    struct pollfd fds[2 * MAX_CONNECTIONS];
    Connection connections[MAX_CONNECTIONS];

    //---------------------------------------------work
    while (true) {
        int newClientSocket = getNewClientSocketOrWait(&localConnectionsCount);

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
                    if (fds[i * 2].revents & POLLHUP) {
                        dropConnectionWrapper(i, "CLIENT_MESSAGE:dead client ", 0,
                                              connections, &localConnectionsCount, threadId);
                    } else if (fds[i * 2].revents & POLLIN) {

                        ssize_t readCount = recv(connections[i].clientSocket, buf, BUFFER_SIZE, 0);

                        if (readCount <= 0) {
                            dropConnectionWrapper(i, "CLIENT_MESSAGE:recv err", 0, connections,
                                                  &localConnectionsCount, threadId);
                            break;
                        }

                        char *newBuffer = allocateMemory(connections[i].buffer, &connections[i].buffer_size,
                                                         (size_t) readCount);
                        if (newBuffer == NULL) {
                            dropConnectionWrapper(i, "buffer error",
                                                  0, connections, &localConnectionsCount, threadId);
                            break;
                        } else { connections[i].buffer = newBuffer; }


                        memcpy(connections[i].buffer, buf, (size_t) readCount);

                        if (connections[i].buffer_size > 3) {
                            char *url = getUrlFromData(connections[i].buffer);

                            if (url != NULL) {
                                if (!isMethodGet(connections[i].buffer)) {
                                    wrongMethod(&connections[i]);
                                    dropConnectionWrapper(i, "CLIENT_MESSAGE:not GET", 0,
                                                          connections, &localConnectionsCount, threadId);
                                    free(url);
                                } else {
                                    int foundInCache = searchCache(url, &connections[i], threadId);
                                    if (1 == foundInCache || 2 == foundInCache) {

                                        connections[i].serverSocket = getServerSocket(url);
                                        free(connections[i].buffer);

                                        connections[i].buffer = createGet(url, &connections[i].buffer_size);

                                        if (connections[i].serverSocket == -1) {
                                            cannotResolve(&connections[i]);
                                            dropConnectionWrapper(i, "CLIENT_MESSAGE:get server err", 0,
                                                                  connections, &localConnectionsCount, threadId);
                                            free(url);
                                            break;
                                        }
                                    }
                                }
                            } else {
                                printf("(%d) (%d)| CLIENT_MESSAGE:not good url\n", threadId, connections[i].id);
                                dropConnectionWrapper(i, "CLIENT_MESSAGE:not good url", 0, connections,
                                                      &localConnectionsCount, threadId);
                            }
                        }
                    }
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

    if (initCache() == -1) {
        printf("ERROR in initCACHE");
        destroyCache();
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

    //---------------------------------work
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