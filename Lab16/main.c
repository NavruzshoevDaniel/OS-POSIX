#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_STRING_SIZE 80


typedef struct node {
    char str[MAX_STRING_SIZE];
    struct node *next;
} node;

void push(node **head_ref, char *new_data) {

    node *new_node = (node *) malloc(sizeof(node));
    strcpy(new_node->str, new_data);


    new_node->next = (*head_ref);

    (*head_ref) = new_node;
}

node *strList;
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int flag = 0;

void notifySortListener(int sign) {
    signal(sign, SIG_IGN);
    if (sign == SIGINT) {
        flag = 1;
    }
    if (sign == SIGALRM) {
        pthread_cond_signal(&cond);
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

void *sortListener(void *args) {
    pthread_mutex_lock(&listMutex);

    while (flag == 0) {
        //bubbleStrSort
        alarm(5);
        pthread_cond_wait(&cond, &listMutex);
    }
    pthread_mutex_unlock(&listMutex);

}



int main() {
    pthread_t pthread;
    //signal(SIGALRM, notifySortListener);
    /*if (pthread_create(&pthread, NULL, sortListener, NULL) != 0) {
        printf("ERROR can't create thread");
        pthread_exit((void *) 0);
    }*/

     node *head = (node *) calloc(1, sizeof(node));
     push(&head, "Abc");
     push(&head, "Bac");
     push(&head, "Cas");
     push(&head, "Dad");
     push(&head, "Gdd");

     bubbleStrSort(head);

    pthread_exit((void *) 0);
}
