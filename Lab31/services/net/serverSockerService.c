//
// Created by Daniel on 26.12.2020.
//
#include <stdio.h>
#include <poll.h>
#include "serverSockerService.h"

/**
 * @return socket descriptor
 *         BIND_EXCEPTION -1;
 *         SOCKET_EXCEPTION -2;
 *         LISTEN_EXCEPTION -3;
 * */
int getProxySocket(int port, int maxConnections) {

    struct sockaddr_in listenAddress;

    listenAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    listenAddress.sin_family = AF_INET;
    listenAddress.sin_port = htons(port);

    int proxySocket = socket(AF_INET, SOCK_STREAM, 0);

    if (proxySocket < 0) {
        perror("Cannot create proxySocket");
        return SOCKET_EXCEPTION;
    }
    int reuse = 1;
    setsockopt(proxySocket, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
    if (bind(proxySocket, (struct sockaddr *) &listenAddress, sizeof(listenAddress))) {
        perror("Cannot bind proxySocket");
        return BIND_EXCEPTION;
    }

    if (listen(proxySocket, maxConnections)) {
        perror("listen error");
        return LISTEN_EXCEPTION;
    }

    return proxySocket;
}

int acceptPollWrapper(struct pollfd *fds,int listenSocket, int amountFds) {
    int polled = poll(fds, amountFds, -1);
    if (polled < 0) {
        printf("poll error");
        return -1;
    } else if (polled == 0) {
        return -1;
    }
    if (fds->revents & POLLIN) {
        return accept(listenSocket, (struct sockaddr *) NULL, NULL);
    } else {
        return -1;
    }
}