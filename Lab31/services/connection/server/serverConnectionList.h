//
// Created by Daniel on 04.01.2021.
//

#ifndef LAB31_SERVERCONNECTIONLIST_H
#define LAB31_SERVERCONNECTIONLIST_H

#include "serverConnection.h"

struct NodeServerConnection {
    ServerConnection *connection;
    struct NodeServerConnection *next;
} typedef NodeServerConnection;


void pushServerConnectionBack(NodeServerConnection **head, ServerConnection *connection);

NodeServerConnection *getServerConnectionNode(NodeServerConnection *head, int n);

void deleteServerConnectionById(NodeServerConnection **head_ref, int key);

void freeServerConnectionList(NodeServerConnection **head);

#endif //LAB31_SERVERCONNECTIONLIST_H
