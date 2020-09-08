#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* func();

int main(){
    pthread_t  pthread;
    int i;
    char *str[] = {"A", "B", "C", "D", "E"};

    if(pthread_create(&pthread, NULL, func, str) != 0){
        return 1;
    }

    sleep(2);

    pthread_cancel(pthread);

    if(pthread_join(pthread, NULL) != 0){
	return 1;
    }

    return 0;
}

void* func(void* ptr){
    char** str;
    int i;
    str = (char **)ptr;

    for(i = 0; i < 5; i++){
        printf("%s\n", str[i]);
	sleep(1);
    }
}

