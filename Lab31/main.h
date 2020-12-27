//
// Created by Daniel on 27.12.2020.
//

#ifndef LAB31_MAIN_H
#define LAB31_MAIN_H

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

int getNewClientSocketOrWait(int *localConnectionsCount, int threadId);

void updatePoll(struct pollfd *fds, int localCount, Connection *connections);

void *work(void *param);

void signalHandler(int sig);

void checkArgs(int argcc, const char *argv[]);

void dropConnectionWrapper(int id,
                           const char *reason,
                           int needToCloseServer,
                           Connection *connections,
                           int *connectionsCount,
                           int threadId);

#endif //LAB31_MAIN_H
