#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mute[3];
int ready = 0;

void* print_message(void* str){
    int k = 1;
    if(0 == ready){
        pthread_mutex_lock(&mute[1]);
        printf("%s:Lock mutex:%d\n", (char*)str, 1);
        ready = 1;
        k = 0;
    }
    for(int i = 0; i < 10 * 3; i++){
        printf("Iteration:%d\n", i);
        if(pthread_mutex_lock(&mute[k]) != 0){
            printf("Err\n");
        }
        else{
            printf("%s:Lock mutex:%d\n", (char*)str, k);
        }
        k = (k + 1) % 3;
        if(pthread_mutex_unlock(&mute[k]) != 0){
            printf("Err\n");
        }
        else{
            printf("%s:Unlock mutex:%d\n", (char*)str, k);
        }
        if(k == 2){
            printf("%s's String\n", (char*)str);
        }
        k = (k + 1) % 3;
    }
    pthread_mutex_unlock(&mute[2]);
    printf("Unlock mutex:%d\n", k);
    return (void*)0;
}

int main(){
    pthread_t pthread;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    for(int i = 0; i < 3; i++){
        pthread_mutex_init(&mute[i], &attr);
    }
    pthread_mutex_lock(&mute[2]);
    printf("Parent:Lock mutex:%d\n", 2);
    pthread_create(&pthread, NULL, print_message, (void*)"Child");

    //Initialization
    while(0 == ready){
        sched_yield();
    }

    //Start work
    print_message("Parent");
    pthread_join(pthread, NULL);
    for(int i = 0; i < 3; i++){
        pthread_mutex_destroy(&mute[i]);
    }
    return 0;
}