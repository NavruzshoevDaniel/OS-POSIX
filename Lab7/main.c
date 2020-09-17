#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ITERATIONS_NUM 20000
int THREADS_NUM = 0;

struct Param{
    int offset;
    double partial_sum;
};

void* execute_thread(void* args){
    int i;
    double pi = 0.0;
    struct Param* param = (struct Param*)args;

    for(i = param->offset; i < ITERATIONS_NUM; i += THREADS_NUM) {
        pi += 1.0/(i*4.0 + 1.0);
        pi -= 1.0/(i*4.0 + 3.0);
    }

    printf("thread-%d's partial_sum = %f\n", param->offset, pi);
    ((struct Param*)args)->partial_sum = pi;

    return args;
}

int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Bad number of arguments!\n");
        return 1;
    }

    THREADS_NUM = atoi(argv[1]);
    pthread_t threads[THREADS_NUM];
    int i;

    struct Param* params;
    params = (struct Param*)malloc(sizeof(struct Param)*THREADS_NUM);

    for(i = 0; i < THREADS_NUM; i++){
        params[i].offset = i;
        if(pthread_create(&threads[i], NULL, execute_thread, &(params[i])) != 0){
            printf("Error while creating thread-%d\n", i);
        }
    }

    double result = 0.0;
    for(i = 0; i < THREADS_NUM; i++){
        if(pthread_join(threads[i], NULL) != 0){
            printf("Error while joining thread-%d\n", i);
        }
        result += (params[i]).partial_sum;
    }

    free(params);
    result = result*4.0;
    printf("pi done = %.15g \n", result);

    return 0;
}
