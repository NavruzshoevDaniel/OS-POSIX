#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

int flag_to_finish = 0;
void* func();
void handler(void* ptr);
void sigAlarm(int sig);

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

void sigAlarm(int sig){
    if(sig == SIGALRM){
        flag_to_finish = 1;
    }
}

void handler(void* ptr){
    printf("Thread is ready to be canceled!\n");
}

void* func(void* ptr){
    char** str;
    int i;
    str = (char **)ptr;
    pthread_cleanup_push(handler, NULL);
    //Not allow printf() to be cancellation point...FUCK YOU, printf()!!!
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    while(flag_to_finish == 0){
        for(i = 0; i < 5; i++){
            printf("%s", str[i]);
        }
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    //Since we execute handler after response on the cancel request
    //We don't need to execute handler again by using non-null value
    //Inside pop-function
    pthread_cleanup_pop(0);
}



