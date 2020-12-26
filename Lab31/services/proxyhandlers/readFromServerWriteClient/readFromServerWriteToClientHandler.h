//
// Created by Daniel on 23.12.2020.
//

#ifndef LAB31_READFROMSERVERWRITETOCLIENTHANDLER_H
#define LAB31_READFROMSERVERWRITETOCLIENTHANDLER_H

#include <poll.h>
#include <sys/socket.h>
#include "../../connection/connection.h"
#include "../../cache/cache.h"
#include "../../net/httpService.h"

#define RECV_FROM_SERVER_EXCEPTION -2
#define SERVER_CLOSED_EXCEPTION -3
#define STATUS_OR_CONTENT_LENGTH_EXCEPTION -4
#define BODY_HTTP_EXCEPTION -5
#define NOT_FREE_CACHE_EXCEPTION -6
#define PUT_CACHE_DATA_EXCEPTION -7

#define END_READING_PROCCESS 1

/**
 * @return 0 - SUCCESS
 *         EMPTY_METHOD -1
 *         RECV_FROM_SERVER_EXCEPTION -2
 *         SERVER_CLOSED_EXCEPTION -3
 *         STATUS_OR_CONTENT_LENGTH_EXCEPTION -4
 *         BODY_HTTP_EXCEPTION -5
 *         NOT_FREE_CACHE_EXCEPTION -6
 *         END_READING_PROCCESS 1
 *
 * */
int handleReadFromServerWriteToClientState(Connection *connection,
                                           struct pollfd clientFd,
                                           struct pollfd serverFd,
                                           CacheInfo *cache,
                                           char *buf,
                                           int bufferSize,
                                           int threadId);

#endif //LAB31_READFROMSERVERWRITETOCLIENTHANDLER_H
