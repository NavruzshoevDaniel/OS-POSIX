//
// Created by Daniel on 04.01.2021.
//

#include "serverConnectionList.h"

void pushServerConnectionBack(NodeServerConnection **head, ServerConnection *connection) {
    while (*head) {
        head = &(*head)->next;
    }
    *head = malloc(sizeof(NodeServerConnection));
    (*head)->connection = connection;
    (*head)->next = NULL;
}

NodeServerConnection *getServerConnectionNode(NodeServerConnection *head, int n) {
    int counter = 0;
    while (counter < n && head) {
        head = head->next;
        counter++;
    }
    return head;
}

void deleteServerConnectionById(NodeServerConnection **head_ref, int key) {

    NodeServerConnection *temp = *head_ref, *prev;

    if (temp != NULL && temp->connection->id == key) {
        *head_ref = temp->next;
        closeServerConnection(temp->connection);
        free(temp);
        return;
    }

    while (temp != NULL && temp->connection->id != key) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return;

    prev->next = temp->next;

    closeServerConnection(temp->connection);
    free(temp);
}

void freeServerConnectionList(NodeServerConnection **head) {
    NodeServerConnection *prev = NULL;
    while ((*head)->next) {
        prev = (*head);
        (*head) = (*head)->next;
        closeServerConnection((*prev).connection);
        free(prev);
    }
    free(*head);
}