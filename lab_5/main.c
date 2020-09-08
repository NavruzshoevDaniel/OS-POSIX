#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* func(void*);
void handler(void*);

int main(){
        pthread_t thread;
        int i;
        char *str1[] = {"A", "6", "B", "2", "C"};

        if(pthread_create(&thread, NULL, func, str1) != 0){
                printf("Error, cannot create thread!\n");
                return 1;
        }

        sleep(2);

        pthread_cancel(thread);

        if(pthread_join(thread, NULL) != 0){
		return 1;
	}

        return 0;
}

void handler(void* ptr){
        printf("Thread prepare to finish...\n");
}

void* func(void* ptr){
        char** str;
        int i;
        str = (char**)ptr;

        pthread_cleanup_push(handler, NULL);

        for(i = 0; i < 5; i++){
                printf("%s\n", str[i]);
                sleep(1);
        }

        pthread_cleanup_pop(0);
}

