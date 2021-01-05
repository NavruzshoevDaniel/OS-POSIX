//
// Created by Daniel on 04.01.2021.
//
#include "clientConnectionList.h"

void pushClientConnectionBack(NodeClientConnection **head, ClientConnection *connection){
    while (*head) {
        head = &(*head)->next;
    }
    *head = malloc(sizeof(NodeClientConnection));
    (*head)->connection = connection;
    (*head)->next = NULL;
}

NodeClientConnection *getClientConnectionNode(NodeClientConnection *head, int n){
    int counter = 0;
    while (counter < n && head) {
        head = head->next;
        counter++;
    }
    return head;
}

void deleteClientConnectionById(NodeClientConnection **head, int key) {

    NodeClientConnection *temp = *head, *prev;

    if (temp != NULL && temp->connection->id == key) {
        *head = temp->next;
        closeClientConnection(temp->connection);
        free(temp);
        return;
    }

    while (temp != NULL && temp->connection->id != key) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return;

    prev->next = temp->next;

    closeClientConnection(temp->connection);
    free(temp);
}

void freeClientConnectionList(NodeClientConnection **head){
    NodeClientConnection *prev = NULL;
    while ((*head)->next) {
        prev = (*head);
        (*head) = (*head)->next;
        closeClientConnection((*prev).connection);
        free(prev);
    }
    free(*head);
}
