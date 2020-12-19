#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#define ITERATIONS_NUM 20000

int THREADS_NUM = 0;
int flag = 0;
int max_iter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

void quit_handler(int sign){
    signal(sign, SIG_IGN);
    if(sign == SIGINT){
        flag = 1;
    }
}

struct Param{
    int offset;
    double partial_sum;
};

void cancel_threads(int thr_num, pthread_t* threads){
    for(int i = 0; i < thr_num; i++){
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
}

void* execute_thread(void* args){

    int i;
    double pi = 0.0;
    struct Param* param = (struct Param*)args;
    size_t iterations = 0;
    size_t com_iter=0;
    for(i = param->offset; ; i += THREADS_NUM) {
        iterations++;
        com_iter++;
        pi += 1.0/(i*4.0 + 1.0);
        pi -= 1.0/(i*4.0 + 3.0);
        if(iterations == ITERATIONS_NUM){
            iterations = 0;
            if(flag == 1){
                pthread_mutex_lock(&mutex);
                if(max_iter!=0){
                   if(max_iter < com_iter){
                        max_iter = com_iter;
                   }
                }
                else{
                   max_iter = com_iter;
                }

                pthread_mutex_unlock(&mutex);
                pthread_barrier_wait(&barrier);

                for(;com_iter < max_iter;i += THREADS_NUM){
                         pi += 1.0/(i*4.0 + 1.0);
                         pi -= 1.0/(i*4.0 + 3.0);
                         com_iter++;
                }

                printf("%d\n",com_iter);
                break;
            }
        }
    }

    printf("thread-%d's partial_sum = %f\n", param->offset, pi);
    ((struct Param*)args)->partial_sum = pi;
    pthread_exit(0);
}

int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Bad number of arguments!\n");
        return 1;
    }

    signal(SIGINT, quit_handler);
    THREADS_NUM = atoi(argv[1]);
    pthread_t threads[THREADS_NUM];
    pthread_barrier_init(&barrier,NULL,THREADS_NUM);
    int stat;
    int i;
    struct Param* params;
    params = (struct Param*)malloc(sizeof(struct Param)*THREADS_NUM);
    for(i = 0; i < THREADS_NUM; i++){
        params[i].offset = i;
        stat = pthread_create(&threads[i], NULL, execute_thread, &(params[i]));
        if(stat != 0){
            printf("Error while creating thread-%d\n", i);
            cancel_threads(THREADS_NUM, threads);
            free(params);
            return 1;
        }
    }
    double result = 0.0;
    for(i = 0; i < THREADS_NUM; i++){
        stat = pthread_join(threads[i], NULL);
        if(stat != 0){
            printf("Error while joining thread-%d\n", i);
        }
        result += (params[i]).partial_sum;
    }
    free(params);
    pthread_barrier_destroy(&barrier);
    result = result*4.0;
    printf("pi done - %.15g \n", result);
    printf("pi orig - %.15g \n",M_PI);
    return 0;
}

