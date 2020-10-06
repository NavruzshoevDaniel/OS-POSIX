#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define PHILOSOPHERS 5
#define DELAY 30000
#define FOOD 50
#define FALSE 0
#define TRUE 1


pthread_mutex_t forks[PHILOSOPHERS];
pthread_mutex_t lForks;
pthread_t philosophers[PHILOSOPHERS];
pthread_cond_t pthreadCond = PTHREAD_COND_INITIALIZER;

void *philosopher(void *num);

int food_on_table();

void get_forks(int, int, int);

void down_forks(int, int);

void cancel_threads(int count, pthread_t *threads);

pthread_mutex_t foodLock;

int main(int argc, char **argv) {
    int i;
    pthread_mutex_init(&foodLock, NULL);
    pthread_mutex_init(&lForks, NULL);

    for (i = 0; i < PHILOSOPHERS; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }

    for (i = 0; i < PHILOSOPHERS; i++) {
        if (pthread_create(&philosophers[i], NULL, philosopher, (void *) i) != 0) {
            printf("Failed to create thread#%d\n", i);
            cancel_threads(PHILOSOPHERS, philosophers);
            pthread_mutex_destroy(&lForks);
            pthread_mutex_destroy(&foodLock);
            for (i = 0; i < PHILOSOPHERS; i++) {
                pthread_mutex_destroy(&forks[i]);
            }
            return 1;
        }
    }

    for (i = 0; i < PHILOSOPHERS; i++) {
        if (pthread_join(philosophers[i], NULL) != 0) {
            printf("Failed to join thread#%d\n", i);
        }
    }

    pthread_mutex_destroy(&lForks);
    pthread_mutex_destroy(&foodLock);
    for (i = 0; i < PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&forks[i]);
    }

    return 0;
}

void *philosopher(void *num) {
    int id;
    int left_fork, right_fork, f;

    id = (int) num;
    printf("Philosopher#%d sit down to dinner.\n", id);
    right_fork = id % PHILOSOPHERS;
    left_fork = (id + 1) % PHILOSOPHERS;

    f = food_on_table();
    while (f) {
        printf("Philosopher#%d: get dinner %d,\n", id, f);
        pthread_mutex_lock(&lForks);
        get_forks(id, left_fork, right_fork);
        pthread_mutex_unlock(&lForks);
        printf("Philosopher#%d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));
        down_forks(left_fork, right_fork);
        f = food_on_table();
    }

    printf("Philosopher %d is done eating.\n", id);
    return NULL;
}

int food_on_table() {
    static int food = FOOD;
    int myFood;
    pthread_mutex_lock(&foodLock);
    if (food > 0) {
        food--;
    }
    myFood = food;
    pthread_mutex_unlock(&foodLock);
    return myFood;
}

void get_forks(int phil, int forkLeft, int forkRight) {
    int getForks = FALSE;
    while (FALSE == getForks) {
        pthread_mutex_lock(&forks[forkLeft]);
        printf("Philosopher#%d: got %s fork %d\n", phil, "left", forkLeft);
        while (FALSE == getForks) {
            if (pthread_mutex_trylock(&forks[forkRight]) == 0) {
                printf("Philosopher#%d: got %s fork %d\n", phil, "right", forkRight);
                getForks = TRUE;
            } else {
                printf("Philosopher#%d: couldn't get %s fork %d\n", phil, "right", forkRight);
                pthread_cond_wait(&pthreadCond, &forks[forkLeft]);
            }
        }
    }
}


void down_forks(int f1, int f2) {
    pthread_mutex_unlock(&forks[f1]);
    pthread_mutex_unlock(&forks[f2]);
    pthread_cond_broadcast(&pthreadCond);
}

void cancel_threads(int count, pthread_t *threads) {
    for (int i = 0; i < count; i++) {
        pthread_cancel(threads[i]);
    }
    for (int i = 0; i < count; i++) {
        pthread_join(threads[i], NULL);
    }
}