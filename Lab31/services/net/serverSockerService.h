//
// Created by Daniel on 26.12.2020.
//

#ifndef LAB31_SERVERSOCKERSERVICE_H
#define LAB31_SERVERSOCKERSERVICE_H


#include <zconf.h>
#include <netinet/in.h>

#define BIND_EXCEPTION -1;
#define SOCKET_EXCEPTION -2;
#define LISTEN_EXCEPTION -3;
/**
 * @return socket descriptor
 *         BIND_EXCEPTION -1;
 *         SOCKET_EXCEPTION -2;
 *         LISTEN_EXCEPTION -3;
 * */
int getProxySocket(int port,int maxConnections);

#endif //LAB31_SERVERSOCKERSERVICE_H