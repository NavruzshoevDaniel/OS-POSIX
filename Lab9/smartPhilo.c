/* 
 * File:   din_phil.c
 * Author: nd159473 (Nickolay Dalmatov, Sun Microsystems)
 * adapted from http://developers.sun.com/sunstudio/downloads/ssx/tha/tha_using_deadlock.html
 *
 * Created on January 1, 1970, 9:53 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define LEFT (i+PHILO-1)%PHILO
#define RIGHT (i+1)%PHILO

pthread_t phils[PHILO];
pthread_t phils_states[PHILO];

void *philosopher(void *id);

int food_on_table();

void get_forks(int phil_i);

void down_forks(int i);

void test(int i);

pthread_mutex_t foodlock;
pthread_mutex_t mutex;
pthread_mutex_t lock;
pthread_cond_t cv[PHILO];

int sleep_seconds = 0;

int main(int argn, char **argv) {
    int i;
    if (argn == 2)
        sleep_seconds = atoi(argv[1]);

    pthread_mutex_init(&foodlock, NULL);
    pthread_mutex_init(&mutex, NULL);

    for (i = 0; i < PHILO; i++)
        pthread_cond_init(&cv[i], NULL);
    for (i = 0; i < PHILO; i++)
        pthread_create(&phils[i], NULL, philosopher, (void *) i);
    for (i = 0; i < PHILO; i++)
        pthread_join(phils[i], NULL);
    return 0;
}

void *philosopher(void *num) {
    int id;
    int f;
    id = (int) num;
    printf("Philosopher %d sitting down to dinner.\n", id);

    while (f = food_on_table()) {


        printf("Philosopher %d: get dish %d.\n", id, f);
        get_forks(id);
        printf("Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));
        printf("Philosopher %d:  not eating.\n", id);
        down_forks(id);
    }
    printf("Philosopher %d is done eating.\n", id);
    return (NULL);
}

int food_on_table() {
    static int food = FOOD;
    int myfood;
    pthread_mutex_lock(&foodlock);
    if (food > 0) {
        food--;
    }
    myfood = food;
    pthread_mutex_unlock(&foodlock);
    return myfood;
}

void get_forks(int phil_i) {
    pthread_mutex_lock(&mutex);
    phils_states[phil_i] = HUNGRY;
    test(phil_i);


    if (phils_states[phil_i] != EATING) {
        printf("Philosopher#%d: Waiting \n", phil_i);
        pthread_cond_wait(&cv[phil_i], &mutex);
    }
    pthread_mutex_unlock(&mutex);
    //printf("Philosopher#%d: not Waiting \n", phil_i);

}

void test(int i) {
    if (phils_states[i] == HUNGRY && phils_states[LEFT] != EATING
        && phils_states[RIGHT] != EATING) {
        phils_states[i] = EATING;
        printf("Philosopher#%d: got %s fork %d\n", i, "left", i);
        printf("Philosopher#%d: got %s fork %d\n", i, "right", (i + PHILO - 1) % PHILO);
        pthread_cond_signal(&cv[i]);
    }
}

void down_forks(int i) {
    pthread_mutex_lock(&mutex);
    phils_states[i] = THINKING;
    printf("Philosopher#%d: put %s fork %d\n", i, "left", i);
    printf("Philosopher#%d: put %s fork %d\n", i, "right", (i + PHILO - 1) % PHILO);
    test(LEFT);
    test(RIGHT);
    pthread_mutex_unlock(&mutex);
}
