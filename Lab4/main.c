#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void* func();

int main(){
    pthread_t  pthread;
    int i;
    void* res;
    char *str[] = {"Operating ", "System ", "Lab ", "4 ", "Test\n"};
    if(pthread_create(&pthread, NULL, func, str) != 0){
        return 1;
    }
    sleep(2);
    pthread_cancel(pthread);
    if(pthread_join(pthread, &res)){
        printf("ERROR, cannot join thread!\n");
    }

    if(res == PTHREAD_CANCELED){
        printf("Thread canceled\n");
    }

    return 0;
}

void* func(void* ptr){
    char** str;
    int i;
    str = (char **)ptr;
    time_t curr;
    curr = time(NULL);
    while(1){
        pthread_testcancel();//Cancelling point
        if(curr < time(NULL)){
            //Print one string every second
            curr = time(NULL);
            for(i = 0; i < 5; i++){
                printf("%s", str[i]);
            }
        }
    }
}

