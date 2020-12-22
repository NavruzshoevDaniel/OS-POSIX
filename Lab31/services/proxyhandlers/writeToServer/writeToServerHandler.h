//
// Created by Daniel on 23.12.2020.
//

#ifndef LAB31_WRITETOSERVERHANDLER_H
#define LAB31_WRITETOSERVERHANDLER_H

#include <sys/socket.h>
#include <poll.h>
#include "../../connection/connection.h"
#include "../../cache/cache.h"

#define SEND_TO_SERVER_EXCEPTION -2

/**
 * @return 0 - success
 *         SEND_TO_SERVER_EXCEPTION -2
 * */
int handleWriteToServerState(Connection *connection,
                             struct pollfd serverFd);

#endif //LAB31_WRITETOSERVERHANDLER_H
