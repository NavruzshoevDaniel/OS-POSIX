#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

sem_t detailA, detailB, detailC, module;

void* createA (void* arg){
    while(1){
        sleep(1);
        sem_post(&detailA);
        printf("Detail A created!\n");
    }
}

void* createB (void* arg){
    while(1){
        sleep(2);
        sem_post(&detailB);
        printf("Detail B created!\n");
    }
}

void* createC (void* arg){
    while(1){
        sleep(3);
        sem_post(&detailC);
        printf("Detail C created!\n");
    }
}

void* createModule (void* arg){
    while(1){
        sem_wait(&detailA);
        sem_wait(&detailB);
        sem_post(&module);
        printf("Module created!\n");
    }
}

void* createWidget (){
    while(1){
        sem_wait(&module);
        sem_wait(&detailC);
        printf("Widget created!\n");
    }
}

int main(){
    pthread_t aThread;
    pthread_t bThread;
    pthread_t cThread;
    pthread_t moduleThread;

    sem_init(&detailA, 0, 0);
    sem_init(&detailB, 0, 0);
    sem_init(&detailC, 0, 0);
    sem_init(&module, 0, 0);

    pthread_create(&aThread, NULL, createA, NULL);
    pthread_create(&bThread, NULL, createB, NULL);
    pthread_create(&cThread, NULL, createC, NULL);
    pthread_create(&moduleThread, NULL, createModule, NULL);
    createWidget();
}