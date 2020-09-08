#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>

#define SIZE 10
#define SUCCESS 0

struct params {
    unsigned lines_count;
    char *lines[SIZE];
};

void *start(void *param) {


    struct params *data = (struct params *) param;
    for (int i = 0; i < data->lines_count; ++i) {
        printf("%s ", data->lines[i]);
    }

    return SUCCESS;
}

int main() {
    pthread_t threads[4];

    struct params thread_params[4] = {
            {4, "Hello", "I", "am", "Dan"},
            {4, "Hello", "I", "am", "Vanya"},
            {4, "Hello", "I", "am", "Leha"},
            {4, "Hello", "I", "am", "Uchitel'"}
    };


    for (int i = 0; i < 4; ++i) {
        int err = pthread_create(&threads[i], NULL, start, &thread_params[i]);
        if (err != 0) {
            exit(err);
        }

    }

    for (int i = 0; i < 4; ++i) {
        int err = pthread_join(threads[i], NULL);
        if (err != 0) {
            exit(err);
        }
    }


    return SUCCESS;
}
