//
// Created by Daniel on 23.12.2020.
//
#include "writeToServerHandler.h"

int handleWriteToServerState(Connection *connection,
                             struct pollfd serverFd) {
    if (serverFd.revents & POLLOUT) {
        if (send(connection->serverSocket, connection->buffer, connection->buffer_size, 0) <= 0) {
            return SEND_TO_SERVER_EXCEPTION;
        }
        setReadFromServerWriteToClientState(connection);
    }
}