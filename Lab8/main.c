#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#define ITERATIONS_NUM 20000
int THREADS_NUM = 0;
int flag = 0;

void quit_handler(int sign){
    signal(sign, SIG_IGN);
    if(sign == SIGINT){
        printf("\n{Flag is true}\n");
        flag = 1;
    }
}

struct Param{
    int offset;
    double partial_sum;
};

void* execute_thread(void* args){
    int i;
    double pi = 0.0;
    struct Param* param = (struct Param*)args;

    size_t iterations = 0;

    for(i = param->offset; ; i += THREADS_NUM) {
        iterations++;
        pi += 1.0/(i*4.0 + 1.0);
        pi -= 1.0/(i*4.0 + 3.0);

        if(iterations == ITERATIONS_NUM){
            iterations = 0;
            if(flag == 1){
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
    int stat;
    int i;

    struct Param* params;
    params = (struct Param*)malloc(sizeof(struct Param)*THREADS_NUM);

    for(i = 0; i < THREADS_NUM; i++){
        params[i].offset = i;
        stat = pthread_create(&threads[i], NULL, execute_thread, &(params[i]));
        if(stat != 0){
            printf("Error while creating thread-%d\n", i);
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

    result = result*4.0;
    printf("pi done - %.15g \n", result);

    return 0;
}
