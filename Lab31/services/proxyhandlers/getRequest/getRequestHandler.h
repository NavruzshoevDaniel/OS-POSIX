//
// Created by Daniel on 20.12.2020.
//

#ifndef LAB31_GETREQUESTHANDLER_H
#define LAB31_GETREQUESTHANDLER_H

#include <poll.h>
#include "../../connection/connection.h"
#include "../../net/httpService.h"
#include "../../cache/cache.h"


#define DEAD_CLIENT_EXCEPTION -2
#define RECV_CLIENT_EXCEPTION -3
#define ALLOCATING_BUFFER_MEMORY_EXCEPTION -4
#define NOT_GET_EXCEPTION -5
#define URL_EXCEPTION -6
#define RESOLVING_SOCKET_FROM_URL_EXCEPTION -7

/**
 * @return 0 - success
 *          DEAD_CLIENT_EXCEPTION -2
 *          RECV_CLIENT_EXCEPTION -3
 *          ALLOCATING_BUFFER_MEMORY_EXCEPTION -4
 *          NOT_GET_EXCEPTION -5
 *          URL_EXCEPTION -6
 * */
int handleGettingRequestState(Connection *connection,
                              char *buf,
                              int bufferSize,
                              int threadId,
                              struct pollfd clientFds,
                              CacheInfo *cache,
                              int maxCacheSize);

/**
 * @return 0 - SUCCESS
 *         RESOLVING_SOCKET_FROM_URL_EXCEPTION -6
 * */
int handleGetMethod(char *url,
                    Connection *connection,
                    CacheInfo *cache,
                    int maxCacheSize,
                    int threadId);

void handleNotResolvingUrl(struct Connection *connection);

void handleNotGetMethod(struct Connection *connection);

#endif //LAB31_GETREQUESTHANDLER_H
