#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void* func();
void handler(void* ptr);

int main(){
    pthread_t  pthread;
    void* res;
    char *str[] = {"Operating ", "System ", "Lab ", "5 ", "Test\n"};
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

void handler(void* ptr){
    printf("Thread is ready to be canceled!\n");
}

void* func(void* ptr){
    char** str;
    int i;
    str = (char **)ptr;
    time_t curr;
    pthread_cleanup_push(handler, NULL);

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
    //Since will execute handler after response on the cancel request
    //We don't need to execute handler again by using non-null value
    //Inside pop-function
    pthread_cleanup_pop(0);
}



