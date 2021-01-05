//
// Created by Daniel on 04.01.2021.
//

#ifndef LAB31_CLIENTCONNECTIONLIST_H
#define LAB31_CLIENTCONNECTIONLIST_H

#include "clientConnection.h"
#include <unistd.h>

struct NodeClientConnection {
    ClientConnection *connection;
    struct NodeClientConnection *next;
} typedef NodeClientConnection;

void pushClientConnectionBack(NodeClientConnection **head, ClientConnection *connection);

NodeClientConnection *getClientConnectionNode(NodeClientConnection *head, int n);

void deleteClientConnectionById(NodeClientConnection **head, int key);

void freeClientConnectionList(NodeClientConnection **head);

#endif //LAB31_CLIENTCONNECTIONLIST_H
