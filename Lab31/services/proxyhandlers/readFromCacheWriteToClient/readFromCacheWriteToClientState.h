//
// Created by Daniel on 26.12.2020.
//

#ifndef LAB31_READFROMCACHEWRITETOCLIENTSTATE_H
#define LAB31_READFROMCACHEWRITETOCLIENTSTATE_H
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include "../../connection/connection.h"
#include "../../cache/cache.h"
#include "../../../config.h"

#define SEND_TO_CLIENT_EXCEPTION -22
#define WRITER_CACHE_INVALID_EXCEPTION -33
#define CACHE_INVALID_EXCEPTION -44
#define SUCCESS_WITH_END 11

int sendNewChunksToClient(Connection *connection, CacheEntry *cache, size_t newSize);

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
                                          const int *localConnectionsCount);
#endif //LAB31_READFROMCACHEWRITETOCLIENTSTATE_H
