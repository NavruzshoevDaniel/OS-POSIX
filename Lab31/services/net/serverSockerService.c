//
// Created by Daniel on 26.12.2020.
//
#include <stdio.h>
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