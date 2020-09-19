#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

int flag_to_finish = 0;
void* func();
void sigAlarm(int sig);

int main(){
    pthread_t  pthread;
    void* res;
    char *str[] = {"Operating ", "System ", "Lab ", "4 ", "Test\n"};
    if(pthread_create(&pthread, NULL, func, str)){
        printf("ERROR, cannot create thread!\n");
        return 1;
    }
    signal(SIGALRM, sigAlarm);
    alarm(2);

    pthread_cancel(pthread);
    if(pthread_join(pthread, &res)){
        printf("ERROR, cannot join thread!\n");
        return 1;
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

void* func(void* ptr){
    char** str;
    int i;
    str = (char **)ptr;
    //Not allow this fucking printf() to cancel thread...
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    while(flag_to_finish == 0){
        for(i = 0; i < 5; i++){
            printf("%s", str[i]);
        }
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
}

