#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_STRING_SIZE 10

typedef struct node {
    char str[MAX_STRING_SIZE];
    struct node *next;
} node;

node *head;
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int flag = 0;

void push(node **head_ref, const char *new_data) {
    node *new_node = (node *) calloc(1, sizeof(node));
    strcpy(new_node->str, new_data);
    new_node->next = (*head_ref);
    (*head_ref) = new_node;
}

void printList(node *);

void notifySortListener(int sign) {

    if (sign == SIGALRM) {
        pthread_cond_signal(&cond);
    }

    if (sign == SIGINT) {
        flag = 1;
        signal(sign, SIG_IGN);
    }
}

void bubbleStrSort(node *listStr) {

    node *node, *nestedNode;

    if (listStr != NULL) {
        while (listStr->next != NULL) {
            node = listStr;
            nestedNode = node->next;
            do {
                if (strcmp(node->str, nestedNode->str) > 0) {
                    char tmp[MAX_STRING_SIZE + 1] = {0};
                    strcpy(tmp, nestedNode->str);
                    strcpy(nestedNode->str, node->str);
                    strcpy(node->str, tmp);
                }
                nestedNode = nestedNode->next;
            } while (nestedNode != NULL);
            listStr = listStr->next;
        }
    }
}

void freeList(node *listStr) {
    node *curNode;
    node *prev;
    curNode = listStr;
    if (listStr != NULL) {
        while (curNode != NULL) {
            prev=curNode;
            curNode = curNode->next;
            free(prev);
        }
    }
}

void *sortListener(void *args) {

    pthread_mutex_lock(&listMutex);

    while (flag == 0) {
        signal(SIGALRM, notifySortListener);
        alarm(5);
        pthread_cond_wait(&cond, &listMutex);
        bubbleStrSort(head);
    }
    pthread_mutex_unlock(&listMutex);
    pthread_exit((void *) 0);
}

void printList(node *listStr) {

    node *next, *curNode;
    curNode = listStr;
    if (listStr != NULL) {
        while (curNode != NULL) {
            printf("%s\n", curNode->str);
            next = curNode->next;
            curNode = next;

        }
    }
}

int main() {
    pthread_t pthread;
    head = (node *) calloc(1, sizeof(node));
    char buffer[MAX_STRING_SIZE];

    if (pthread_create(&pthread, NULL, sortListener, NULL) != 0) {
        printf("ERROR can't create thread");
        pthread_exit((void *) 0);
    }


    while (flag == 0) {
        int count = read(0, buffer, MAX_STRING_SIZE - 1);

        if (strcmp(buffer, "\n") == 0) {
            pthread_mutex_lock(&listMutex);
            printList(head);
            pthread_mutex_unlock(&listMutex);
            memset(buffer, 0, MAX_STRING_SIZE);
            continue;
        }
        if (buffer[count - 1] == '\n') {
            buffer[count - 1] = 0;
        }

        pthread_mutex_lock(&listMutex);
        push(&head, buffer);
        pthread_mutex_unlock(&listMutex);
        memset(buffer, 0, MAX_STRING_SIZE);
    }
    freeList(head);
    pthread_exit((void *) 0);
}
