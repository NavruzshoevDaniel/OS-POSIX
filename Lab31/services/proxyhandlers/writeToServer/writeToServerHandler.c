//
// Created by Daniel on 23.12.2020.
//
#include "writeToServerHandler.h"

/**
 * @return 0 - success
 *         SEND_TO_SERVER_EXCEPTION -2
 * */
int handleWriteToServerState(Connection *connection,
                             struct pollfd serverFd) {
    if (serverFd.revents & POLLOUT) {
        if (send(connection->serverSocket, connection->buffer, connection->buffer_size, 0) <= 0) {
            perror("send to server");
            return SEND_TO_SERVER_EXCEPTION;
        }
        setReadFromServerWriteToClientState(connection);
    }
    return EXIT_SUCCESS;
}