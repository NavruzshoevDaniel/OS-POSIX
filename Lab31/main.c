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
#include "services/queue/queueService.h"
#include "services/threadpool/threadPool.h"
#include "argschecker/argsChecker.h"
#include "services/connection/connection.h"
#include "services/concurrent/atomicInt.h"

int DEFAULT_PORT;
#define MAX_CONNECTIONS 100
#define MAX_CACHE_SIZE 1024
#define BUFFER_SIZE 16 * 1024
#define BACKLOG 100
#define CHUNK 64
#define MAX_CACHE_ENTRY_SIZE 101 * 1048576
#define MAX_CONNECTIONS_PER_THREAD allConnectionsCount / threadsCount

static int allConnectionsCount = 0;
static int cacheSize = 0;
static int proxySocket;
Queue *socketQueue;
int threadsCount;
pthread_t *threads;
int *threadsId;

pthread_mutex_t connectionsMutex;

enum cacheEntryStatus {
    DOWNLOADING,
    VALID,
    INVALID
};

struct cacheEntry {
    size_t allSize;
    size_t alreadyWritten;

    pthread_mutex_t mutex;
    pthread_cond_t condVar;

    int writerId;
    char *url;
    char *data;
    enum cacheEntryStatus status;
};

static struct cacheEntry cache[MAX_CACHE_SIZE];

int isMethodGet(char *httpData) {
    return httpData[0] == 'G' &&
           httpData[1] == 'E' &&
           httpData[2] == 'T' &&
           httpData[4] == 'h' &&
           httpData[5] == 't' &&
           httpData[6] == 't' &&
           httpData[7] == 'p';
}

char *getUrlFromData(char *httpData) {
    char *startUrl = strchr(httpData, ' ');
    if (startUrl == NULL) {
        return NULL;
    }

    startUrl++;
    char *endUrl = strchr(startUrl, ' ');
    if (endUrl == NULL) {
        return NULL;
    }
    char *result = malloc(sizeof(char) * (endUrl - startUrl + 1));

    memcpy(result, startUrl, (size_t) (endUrl - startUrl));
    result[endUrl - startUrl] = '\0';

    return result;
}

char *getHostFromUrl(char *url) {
    url += 7;

    char *endHost = strchr(url, '/');

    char *result = malloc(sizeof(char) * (endHost - url + 1));

    memcpy(result, url, (size_t) (endHost - url));
    result[endHost - url] = '\0';

    return result;
}

int getResponseCodeFromData(char *httpData) {
    httpData += 9;

    char *end;

    int response = (int) strtol(httpData, &end, 10);
    if (end == httpData || response <= 0) {
        return -1;
    }
    return response;
}

long getContentLengthFromData(char *httpData) {
    char *header = "Content-Length:";

    char *startLength = strstr(httpData, header);
    if (startLength == NULL) {
        return -1;
    }

    startLength += strlen(header) + 1;
    char *end;
    long contentLength = strtol(startLength, &end, 10);
    if (end == startLength || contentLength <= 0) {
        return -1;
    }

    return contentLength;
}

int isResponseHasPayload(int statusCode) {
    if (statusCode == 204) {
        return 0;
    }

    if (statusCode == 304) {
        return 0;
    }

    if (statusCode >= 100 && statusCode < 200) {
        return 0;
    }

    return 1;
}

void editRequestToSendToServer(Connection *connection) {
    char *startUrl = strchr(connection->buffer, ' ');
    char *endHost = strchr(startUrl + 8, '/');

    memmove(startUrl + 1, endHost, connection->buffer_size - (endHost - connection->buffer));
    connection->buffer_size -= endHost - (startUrl + 1);

    char *protocol_1_1 = strstr(connection->buffer, "HTTP/1.1");
    char *protocol_2_0 = strstr(connection->buffer, "HTTP/2.0");

    if (protocol_1_1 != NULL) {
        protocol_1_1[7] = '0';
    }
    if (protocol_2_0 != NULL) {
        protocol_2_0[5] = '1';
    }
}

int getProxySocket(struct sockaddr_in listenAddress) {
    int proxySocket = socket(AF_INET, SOCK_STREAM, 0);


    if (proxySocket < 0) {
        perror("Cannot create proxySocket");
        exit(EXIT_FAILURE);
    }

    if (bind(proxySocket, (struct sockaddr *) &listenAddress, sizeof(listenAddress))) {
        perror("Cannot bind proxySocket");
        exit(EXIT_FAILURE);
    }

    if (listen(proxySocket, BACKLOG)) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    return proxySocket;
}

struct sockaddr_in getListenAddress() {
    struct sockaddr_in result;

    result.sin_addr.s_addr = htonl(INADDR_ANY);
    result.sin_family = AF_INET;
    result.sin_port = htons(DEFAULT_PORT);

    return result;
}

int getServerSocket(char *url) {
    char *host = getHostFromUrl(url);
    struct hostent *hostInfo = gethostbyname(host);
    free(host);

    if (NULL == hostInfo) {
        fprintf(stderr, "Cannot get host by name\n");
        return -1;
    }

    struct sockaddr_in destinationAddress;

    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = htons(80);
    memcpy(&destinationAddress.sin_addr, hostInfo->h_addr, hostInfo->h_length);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

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


void updatePollFds(struct pollfd *fds, int connectionsCount, Connection *connections) {
    for (int i = 0; i < connectionsCount; ++i) {
        fds[i * 2].fd = connections[i].clientSocket;
        fds[i * 2 + 1].fd = connections[i].serverSocket;
        switch (connections[i].status) {
            case GETTING_REQUEST_FROM_CLIENT:
                fds[i * 2].events = POLLIN;
                fds[i * 2 + 1].events = 0;
                break;
            case CONNECTING_TO_SERVER:
                fds[i * 2].events = 0;
                fds[i * 2 + 1].events = POLLOUT;
                break;
            case WRITING_REQUEST:
                fds[i * 2].events = 0;
                fds[i * 2 + 1].events = POLLOUT;
                break;
            case FORWARDING_REQUEST:
                fds[i * 2].events = POLLIN | POLLRDBAND | POLLRDNORM;
                fds[i * 2 + 1].events = POLLOUT;
                break;
            case FORWARDING_RESPONSE:
                fds[i * 2].events = POLLOUT;
                fds[i * 2 + 1].events = POLLIN;
                break;
            case RESPONDING_FROM_CACHE:
                fds[i * 2].events = POLLOUT;
                fds[i * 2 + 1].events = 0;
                break;
            case WAITING_DATA_ON_CACHE:
                fds[i * 2].events = 0;
                fds[i * 2 + 1].events = 0;
            case NOT_ACTIVE:
                break;
        }
    }
}

void wrongMethod(struct Connection *connection) {
    char *wrong = "HTTP: 405\r\nAllow: GET\r\n";
    write(connection->clientSocket, wrong, 23);
}

void cannotResolve(struct Connection *connection) {
    char *errorstr = "HTTP: 523\r\n";
    write(connection->clientSocket, errorstr, 11);
}

int searchCache(char *url, struct Connection *connection, int threadId) {
    connection->cacheIndex = -1;

    for (int j = 0; j < cacheSize; ++j) {
        if (strcmp(cache[j].url, url) == 0) {
            if (cache[j].status == VALID) {
                printf("Thread: %d, Connection %d, Found cache entry for %s, responding from cache\n", threadId,
                       connection->id, url);
                freeConnectionBuffer(connection);
            } else if (cache[j].status == DOWNLOADING) {
                printf("Thread: %d, Connection %d, Found cache entry for %s, downloading by another Connection, responding from cache\n",
                       threadId, connection->id, url);
            } else if (cache[j].status == INVALID) {
                cache[j].status = DOWNLOADING;
                connection->cacheIndex = j;
                cache[j].writerId = threadId;
                free(cache[j].data);
                cache[j].alreadyWritten = 0;
                cache[j].allSize = 0;
                return 0;
            }
            freeConnectionBuffer(connection);
            connection->buffer_size = 0;
            connection->cacheIndex = j;
            connection->cacheBytesWritten = 0;
            connection->status = RESPONDING_FROM_CACHE;

            free(url);
            return 1;
        }
    }
    if (cacheSize >= MAX_CACHE_SIZE) {
        connection->cacheIndex = -1;
    } else if (connection->cacheIndex == -1) {
        cacheSize++;
        connection->cacheIndex = cacheSize - 1;
        cache[connection->cacheIndex].status = DOWNLOADING;
        cache[connection->cacheIndex].writerId = threadId;
        cache[connection->cacheIndex].url = url;
        cache[connection->cacheIndex].alreadyWritten = 0;
        cache[connection->cacheIndex].allSize = 0;
        initMutex(&cache[connection->cacheIndex].mutex);
        initCondVariable(&cache[connection->cacheIndex].condVar);
    }

    return 0;
}

void cutBody(char *buff, size_t *len, size_t newLength) {
    buff = realloc(buff, newLength);
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

char *allocateMemory(char *buf, size_t *length, size_t additionalCount) {
    if (*length == 0) {
        *length = additionalCount;
        buf = malloc(*length * sizeof(char));
    } else {
        *length += (size_t) additionalCount;
        buf = realloc(buf, *length);
    }
    if (buf == NULL) {
        printf("allocation memory failed in method allocateMemory() %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return buf;
}

static void exitHandler(void) {
    close(proxySocket);
    for (int i = 0; i < cacheSize; ++i) {
        free(cache[i].data);
    }

    free(threadsId);
    free(threads);
    clearQueue(socketQueue);
}

void makeCacheInvalid(int id) {
    pthread_mutex_lock(&cache[id].mutex);
    cache[id].status = INVALID;
    cache[id].writerId = -1;
    pthread_cond_broadcast(&cache[id].condVar);
    pthread_mutex_unlock(&cache[id].mutex);
}

void wakeUp(struct Connection *connections, int connectionsCount) {
    for (int i = 0; i < connectionsCount; ++i) {
        if (connections[i].status == WAITING_DATA_ON_CACHE) {
            connections[i].status = RESPONDING_FROM_CACHE;
        }
    }
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

//TODO:: refract it by 22-24 labs
int getFreeSocketOrWait(int *ownThreadConnectionsCount) {
    int newClientSocket = -1;
    pthread_mutex_lock(&socketQueue->checkSizeMutex);
    pthread_mutex_lock(&connectionsMutex);
    if (MAX_CONNECTIONS_PER_THREAD >= *ownThreadConnectionsCount && !isEmpty(socketQueue)) {
        newClientSocket = getSocketFromQueue(socketQueue);
        if (newClientSocket != -1) {
            (*ownThreadConnectionsCount)++;
        }
    }
    pthread_mutex_unlock(&connectionsMutex);

    while (ownThreadConnectionsCount == 0 && isEmpty(socketQueue)) {

        pthread_cond_wait(&socketQueue->condVar, &socketQueue->checkSizeMutex);

        newClientSocket = getSocketFromQueue(socketQueue);
        if (newClientSocket != -1) {
            (*ownThreadConnectionsCount)++;
        }
    }
    pthread_mutex_unlock(&socketQueue->checkSizeMutex);
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
    allConnectionsCount = decrementAndGet(allConnectionsCount, &connectionsMutex);
}

void handleGettingRequest(Connection *connections,
                          const struct pollfd *fds,
                          int *ownThreadConnectionsCount,
                          char *buf,
                          int threadId,
                          int connectionIndex) {
    if (fds[connectionIndex * 2].revents & POLLHUP) {
        dropConnectionWrapper(connectionIndex, "closed by client", 0, connections, ownThreadConnectionsCount, threadId);
    } else if (fds[connectionIndex * 2].revents & POLLIN) {
        printf("00\n");
        ssize_t readCount = recv(connections[connectionIndex].clientSocket, buf, BUFFER_SIZE, 0);

        if (readCount <= 0) {
            dropConnectionWrapper(connectionIndex, "Connection broken", 0, connections, ownThreadConnectionsCount,
                                  threadId);
            return;
        }
        printf("01\n");

        connections[connectionIndex].buffer = allocateMemory(connections[connectionIndex].buffer,
                                                             &connections[connectionIndex].buffer_size,
                                                             (size_t) readCount);
        char *dest = connections[connectionIndex].buffer + connections[connectionIndex].buffer_size - readCount;
        memcpy(dest, buf, (size_t) readCount);
        printf("02\n");
        if (connections[connectionIndex].buffer_size > 3) {
            char *url = getUrlFromData(connections[connectionIndex].buffer);
            printf("03\n");
            if (url != NULL) {
                if (!isMethodGet(connections[connectionIndex].buffer)) {
                    wrongMethod(&connections[connectionIndex]);
                    dropConnectionWrapper(connectionIndex, "Only GET methods allowed, Connection dropped\n", 0,
                                          connections,
                                          ownThreadConnectionsCount, threadId);
                    allConnectionsCount = decrementAndGet(allConnectionsCount, &connectionsMutex);
                    free(url);
                } else {
                    int foundInCache = searchCache(url, &connections[connectionIndex], threadId);
                    printf("04\n");
                    if (!foundInCache) {
                        printf("Thread: %d, Connection %d, Cache entry for %s not found, responding from server\n",
                               threadId, connections[connectionIndex].id, url);
                        connections[connectionIndex].status = CONNECTING_TO_SERVER;
                        connections[connectionIndex].serverSocket = getServerSocket(url);
                        if (connections[connectionIndex].serverSocket == -1) {
                            cannotResolve(&connections[connectionIndex]);
                            dropConnectionWrapper(connectionIndex, "Connection %d, Cannot resolve host\n", 0,
                                                  connections,
                                                  ownThreadConnectionsCount, threadId);
                            allConnectionsCount = decrementAndGet(allConnectionsCount, &connectionsMutex);
                            return;
                        }
                        editRequestToSendToServer(&connections[connectionIndex]);
                    }
                    printf("05\n");
                }
            }
        }
    }
}

void *work(void *param) {
    int threadId = *((int *) param);

    printf("id: %d\n", threadId);

    int ownThreadConnectionsCount = 0;
    Connection connections[MAX_CONNECTIONS];

    while (1 >= 1 - 1 + 1) {
        int newClientSocket = getFreeSocketOrWait(&ownThreadConnectionsCount);

        if (newClientSocket != -1) {
            initNewConnection(&connections[ownThreadConnectionsCount - 1], newClientSocket);
        }

        struct pollfd fds[2 * MAX_CONNECTIONS];
        updatePollFds(fds, ownThreadConnectionsCount, connections);

        int polled = poll(fds, ownThreadConnectionsCount * 2, -1);

        if (polled < 0) {
            perror("poll error");
        } else if (polled == 0) {
            continue;
        }

        char buf[BUFFER_SIZE];
        for (int i = 0; i < ownThreadConnectionsCount; ++i) {
            switch (connections[i].status) {
                case GETTING_REQUEST_FROM_CLIENT:
                    handleGettingRequest(connections, fds, &ownThreadConnectionsCount, buf, threadId, i);
                    break;
                case CONNECTING_TO_SERVER:
                    if (fds[i * 2 + 1].revents & POLLOUT) {
                        printf("CONNECTING TO SERVER\n");
                        connections[i].status = WRITING_REQUEST;
                        printf("END OF CONNECTING TO SERVER\n");
                    }
                    break;
                case WRITING_REQUEST:
                    if (fds[i * 2 + 1].revents & POLLOUT) {

                        int indexOfBody = getIndexOfBody(connections[i].buffer, connections[i].buffer_size);
                        if (indexOfBody != -1) {

                            //                          printf("WRITING REQUEST CUTBODY()\n");

                            cutBody(connections[i].buffer, &connections[i].buffer_size, (size_t) (indexOfBody + 1));
                            if (send(connections[i].serverSocket, connections[i].buffer, connections[i].buffer_size,
                                     0) < 0) {
                                makeCacheInvalid(i);
                                wakeUp(connections, ownThreadConnectionsCount);
                                dropConnectionWrapper(i, "Failed to write request", 1, connections,
                                                      &ownThreadConnectionsCount,
                                                      threadId);
                                break;
                            }
                            freeConnectionBuffer(&connections[i]);
                            connections[i].status = FORWARDING_RESPONSE;
                            //                         printf("WRITING REQUEST\n");
                        } else {
                            if (send(connections[i].serverSocket, connections[i].buffer, connections[i].buffer_size,
                                     0) < 0) {
                                makeCacheInvalid(i);
                                wakeUp(connections, ownThreadConnectionsCount);
                                dropConnectionWrapper(i, "Failed to write request", 1, connections,
                                                      &ownThreadConnectionsCount,
                                                      threadId);
                                break;
                            }
                            connections[i].status = FORWARDING_REQUEST;
                        }
                    }
                    break;
                case FORWARDING_REQUEST:
                    if ((fds[i * 2].revents & POLLRDNORM || fds[i * 2].revents & POLLRDBAND ||
                         fds[i * 2].revents & POLLIN) && fds[i * 2 + 1].revents & POLLOUT) {
                        ssize_t readCount = recv(connections[i].clientSocket, buf, BUFFER_SIZE, 0);

                        if (readCount > 0) {

                            //                          printf("FORWARDING REQUEST CUTBODY()\n");

                            int indexOfBody = getIndexOfBody(buf, (size_t) readCount);
                            if (indexOfBody != -1) {
                                cutBody(buf, (size_t *) &readCount, (size_t) (indexOfBody + 1));
                            }

                            connections[i].buffer = allocateMemory(connections[i].buffer, &connections[i].buffer_size,
                                                                   readCount);

                            char *dest = connections[i].buffer + connections[i].buffer_size - readCount;
                            memcpy(dest, buf, (size_t) readCount);

                            //                         printf("FORWARDING REQUEST\n");

                            if (indexOfBody >= 0) {
                                connections[i].status = FORWARDING_RESPONSE;
                                freeConnectionBuffer(&connections[i]);
                            }

                            if (send(connections[i].serverSocket, buf, (size_t) readCount, 0) < 0) {
                                dropConnectionWrapper(i, "Failed to forwarding response", 1, connections,
                                                      &ownThreadConnectionsCount,
                                                      threadId);
                            }
                        } else {
                            dropConnectionWrapper(i, "Failed to getSocketFromQueue request from client", 1, connections,
                                                  &ownThreadConnectionsCount, threadId);
                        }
                    }
                    break;
                case FORWARDING_RESPONSE:
                    if (fds[i * 2].revents & POLLOUT && fds[i * 2 + 1].revents & POLLIN) {
                        //                       printf("FORWARDING RESPONSE\n");
                        ssize_t readCount = recv(connections[i].serverSocket, buf, BUFFER_SIZE, 0);

                        if (readCount < 0) {
                            dropConnectionWrapper(i, "Connection broken", 1, connections, &ownThreadConnectionsCount,
                                                  threadId);
                            break;
                        }

                        if (readCount == 0) {
                            if (connections[i].cacheIndex != -1) {
                                makeCacheInvalid(connections[i].cacheIndex);
                                wakeUp(connections, ownThreadConnectionsCount);
                            }
                            dropConnectionWrapper(i, "Transmission over, socket closed", 1, connections,
                                                  &ownThreadConnectionsCount,
                                                  threadId);
                            break;
                        }

                        if (send(connections[i].clientSocket, buf, (size_t) readCount, 0) < 0) {
                            makeCacheInvalid(connections[i].cacheIndex);
                            wakeUp(connections, ownThreadConnectionsCount);
                            dropConnectionWrapper(i, "Client closed Connection", 1, connections,
                                                  &ownThreadConnectionsCount,
                                                  threadId);
                            break;
                        }

                        if (connections[i].cacheIndex == -1) {
                            break;
                        }

                        if (cache[connections[i].cacheIndex].alreadyWritten == 0) {
                            connections[i].buffer = allocateMemory(connections[i].buffer, &connections[i].buffer_size,
                                                                   (size_t) readCount);

                            char *dest = connections[i].buffer + connections[i].buffer_size - readCount;
                            memcpy(dest, buf, (size_t) readCount);

                            int indexOfBody = getIndexOfBody(connections[i].buffer, connections[i].buffer_size);
                            if (indexOfBody >= 0) {
                                int statusCode = getResponseCodeFromData(connections[i].buffer);
                                long contentLength = getContentLengthFromData(connections[i].buffer);

                                if (contentLength >= MAX_CACHE_ENTRY_SIZE) {
                                    makeCacheInvalid(connections[i].cacheIndex);
                                    wakeUp(connections, ownThreadConnectionsCount);
                                    freeConnectionBuffer(&connections[i]);
                                    break;
                                }

                                if (contentLength != -1) {
                                    if (statusCode != 200) {
                                        makeCacheInvalid(connections[i].cacheIndex);
                                        wakeUp(connections, ownThreadConnectionsCount);
                                    } else {
                                        cache[connections[i].cacheIndex].allSize = (size_t) (contentLength +
                                                                                             indexOfBody);
                                        cache[connections[i].cacheIndex].data = malloc(
                                                cache[connections[i].cacheIndex].allSize);

                                        if (cache[connections[i].cacheIndex].data == NULL) {
                                            printf("malloc failed\n");
                                            connections[i].cacheIndex = -1;
                                        } else {
                                            pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
                                            cache[connections[i].cacheIndex].alreadyWritten = connections[i].buffer_size;
                                            memcpy(cache[connections[i].cacheIndex].data, connections[i].buffer,
                                                   cache[connections[i].cacheIndex].alreadyWritten);
                                            pthread_cond_broadcast(&cache[connections[i].cacheIndex].condVar);
                                            pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);
                                        }
                                        wakeUp(connections, ownThreadConnectionsCount);

                                        if (cache[connections[i].cacheIndex].alreadyWritten ==
                                            cache[connections[i].cacheIndex].allSize) {

                                            cache[connections[i].cacheIndex].status = VALID;
                                            dropConnectionWrapper(i, "Transmission over, data transfered succesfully",
                                                                  1,
                                                                  connections, &ownThreadConnectionsCount, threadId);
                                        }
                                    }
                                    freeConnectionBuffer(&connections[i]);
                                } else {
                                    makeCacheInvalid(connections[i].cacheIndex);
                                    wakeUp(connections, ownThreadConnectionsCount);
                                    if (!isResponseHasPayload(statusCode)) {
                                        dropConnectionWrapper(i, "Transmission over, no payload for response", 1,
                                                              connections,
                                                              &ownThreadConnectionsCount, threadId);
                                        printf("Status code: %d\n", statusCode);
                                    }
                                }
                                break;
                            }

                        } else {
                            pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
                            cache[connections[i].cacheIndex].alreadyWritten += readCount;
                            char *dest = cache[connections[i].cacheIndex].data +
                                         cache[connections[i].cacheIndex].alreadyWritten - readCount;
                            memcpy(dest, buf, (size_t) readCount);
                            pthread_cond_broadcast(&cache[connections[i].cacheIndex].condVar);
                            pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);
                            wakeUp(connections, ownThreadConnectionsCount);

                            if (cache[connections[i].cacheIndex].alreadyWritten ==
                                cache[connections[i].cacheIndex].allSize) {

                                cache[connections[i].cacheIndex].status = VALID;
                                dropConnectionWrapper(i, "Transmission over, data transfered succesfully", 1,
                                                      connections,
                                                      &ownThreadConnectionsCount, threadId);
                            }
                        }
                    }
                    break;

                case RESPONDING_FROM_CACHE:
                    if (fds[i * 2].revents & POLLOUT) {
                        int flag = 0;

                        if (cache[connections[i].cacheIndex].status == INVALID) {
                            pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);
                            dropConnectionWrapper(i, "Failed to load from cache", 0, connections,
                                                  &ownThreadConnectionsCount,
                                                  threadId);
                            break;
                        }

                        if ((cache[connections[i].cacheIndex].alreadyWritten == connections[i].cacheBytesWritten ||
                             cache[connections[i].cacheIndex].allSize == 0) &&
                            cache[connections[i].cacheIndex].writerId == threadId) {

                            printf("WAITING DATA ON CACHE\n");
                            connections[i].status = WAITING_DATA_ON_CACHE;
                            break;
                        }

                        if (cache[connections[i].cacheIndex].status != VALID) {
                            if (cache[connections[i].cacheIndex].status == DOWNLOADING) {
                                pthread_mutex_lock(&cache[connections[i].cacheIndex].mutex);
                                while (cache[connections[i].cacheIndex].alreadyWritten ==
                                       connections[i].cacheBytesWritten ||
                                       cache[connections[i].cacheIndex].allSize == 0) {
                                    if (cache[connections[i].cacheIndex].status == INVALID) {
                                        pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);
                                        dropConnectionWrapper(i, "Failed to load from cache", 0, connections,
                                                              &ownThreadConnectionsCount, threadId);
                                        flag = 1;
                                        break;
                                    }
                                    pthread_cond_wait(&cache[connections[i].cacheIndex].condVar,
                                                      &cache[connections[i].cacheIndex].mutex);
                                }
                                pthread_mutex_unlock(&cache[connections[i].cacheIndex].mutex);
                            } else if (cache[connections[i].cacheIndex].status == INVALID) {
                                dropConnectionWrapper(i, "Failed to load from cache", 0, connections,
                                                      &ownThreadConnectionsCount,
                                                      threadId);
                                flag = 1;
                            }
                        }
                        if (1 == flag) {
                            break;
                        }

                        struct cacheEntry entry = cache[connections[i].cacheIndex];

                        char *sendData = entry.data + connections[i].cacheBytesWritten;
                        size_t bytesToWrite = entry.alreadyWritten - connections[i].cacheBytesWritten;

                        if (bytesToWrite > CHUNK) {
                            bytesToWrite = CHUNK;
                        }
                        ssize_t bytesWritten = send(connections[i].clientSocket, sendData, bytesToWrite, 0);

                        if (bytesWritten < 0) {
                            dropConnectionWrapper(i, "Connection broken by client", 0, connections,
                                                  &ownThreadConnectionsCount,
                                                  threadId);
                            break;
                        }

                        connections[i].cacheBytesWritten += bytesWritten;

                        if (connections[i].cacheBytesWritten == cache[connections[i].cacheIndex].allSize) {
                            dropConnectionWrapper(i, "Transmission over, loaded from cache", 0, connections,
                                                  &ownThreadConnectionsCount, threadId);
                        }
                    }
                case NOT_ACTIVE:
                    break;
                case WAITING_DATA_ON_CACHE:
                    break;
            }
        }
    }
}

int main(int argc, const char *argv[]) {
    checkCountArguments(argc);

    scanf("%d", &DEFAULT_PORT);

    atexit(exitHandler);
    initMutex(&connectionsMutex);

    char *end;
    threadsCount = strtol(argv[1], &end, 10);
    checkIfValidParsedInt(threadsCount);
    threadsId = malloc(threadsCount * sizeof(int));

    socketQueue = createQueue();
    threads = createThreadPool(work, threadsId, threadsCount);

    proxySocket = getProxySocket(getListenAddress());
    signal(SIGPIPE, SIG_IGN);

    while (1) {
        int connectedSocket = accept(proxySocket, (struct sockaddr *) NULL, NULL);

        if (connectedSocket != -1) {
            printf("ACCEPTED\n");

            pthread_mutex_lock(&socketQueue->checkSizeMutex);
            putSocketInQueue(socketQueue, connectedSocket);
            pthread_mutex_unlock(&socketQueue->checkSizeMutex);

            allConnectionsCount = incrementAndGet(allConnectionsCount, &connectionsMutex);
        }
    }
}
