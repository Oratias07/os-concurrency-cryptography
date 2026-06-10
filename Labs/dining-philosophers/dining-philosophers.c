#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#define LEFT(i,N)  ((i + N - 1) % N)    //macro defining the left neighbor index
#define RIGHT(i,N)  ((i + 1) % N)       //macro defining the right neighbor index
#define THINKING 0                       //philosopher is thinking
#define HUNGRY 1                         //philosopher is hungry (waiting for sticks)
#define EATING 2                         //philosopher is eating
#define MAX_TIME 5                       //maximum time allowed for eating and/or thinking

int N;                  //number of philosophers
sem_t *mutex = NULL;    //pointer to binary semaphore
sem_t *eaters = NULL;   //array of semaphores, one per philosopher
int* state = NULL;      //array of integers representing each philosopher's state (eating, hungry, thinking)


//eating process
void eat(int i) {
    int time = rand() % MAX_TIME;
    printf("Philosopher #%d is eating...\n", i + 1);
    sleep(time);
    printf("Philosopher #%d stopped eating...\n", i + 1);
}

//thinking process
void think(int i) {
    int time = rand() % MAX_TIME;
    printf("Philosopher #%d is thinking...\n", i + 1);
    sleep(time);
}

//check whether a hungry philosopher can start eating
void test(int i) {
    //check if the philosopher is hungry and neighbors are not eating (sticks are available)
    if (state[i] == HUNGRY) {
        if (!(state[LEFT(i,N)] == EATING) && !(state[RIGHT(i, N)] == EATING)) {
            state[i] = EATING;       //update status - philosopher is now eating
            //philosopher started eating, signal his semaphore so neighbors won't block him
            sem_post(&eaters[i]);
        }
    }
}

//philosopher attempts to pick up sticks
void take_sticks(int i) {
    sem_wait(mutex);         //enter critical section - only one philosopher can pick up sticks at a time
    state[i] = HUNGRY;       //philosopher is hungry
    test(i);                 //attempt to pick up both sticks using test function
    sem_post(mutex);         //leave critical section
    sem_wait(&eaters[i]);    //if failed to start eating, block. neighbors will unblock after they finish eating
}

//philosopher puts down sticks
void put_sticks(int i) {
    sem_wait(mutex);         //enter critical section - only one philosopher works with sticks at a time
    state[i] = THINKING;     //philosopher finished eating, starts thinking
    test(LEFT(i, N));        //check if left neighbor is hungry and waiting for sticks, signal him to start eating
    test(RIGHT(i, N));       //same for right neighbor
    sem_post(mutex);         //release critical section
}

//this code runs on each thread, describing a philosopher's behavior
void* philosopher(void* arg) {
    int i = *((int*)arg);
    while (1 == 1) {        //infinite loop
        think(i);           //philosopher thinks first
        take_sticks(i);     //then attempts to pick up sticks
        eat(i);             //eats
        put_sticks(i);      //puts sticks back on the table
    }
}


int main(int argc, char* argv[]) {
    void* result;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <NUMBER_OF_PHILOSOPHERS>\n", argv[0]);
        return 1;
    }

    //preparation...
    N = atoi(argv[1]);    //convert number of philosophers from string to integer

    if (N <= 1) {
        fprintf(stderr, "Error by transformation of the argument...\n");
        return 2;
    }    //no fewer than 2 philosophers

    mutex = (sem_t*)malloc(sizeof(sem_t));
    if (sem_init(mutex, 0, 1) != 0) {    //create mutex to separate entry into critical section
        fprintf(stderr, "Error by creating semaphore...\n");
        return 3;
    }

    eaters = (sem_t*)malloc(sizeof(sem_t) * N);    //allocate memory for semaphores - one per philosopher
    state = (int*)malloc(sizeof(int) * N);          //allocate memory for state array - each integer represents a philosopher's state (hungry, eating, thinking)

    memset(state, 0, N);

    srand(time(NULL));
    pthread_t *philosophers = (pthread_t*)malloc(N * sizeof(pthread_t));    //allocate memory for threads according to number of philosophers

    int i;
    for (i = 0; i < N; i++) {
        if (sem_init(&eaters[i], 0, 0) != 0) {    //initialize philosopher semaphores
            fprintf(stderr, "Error by creating semaphore...\n");
            return 3;
        }
    }

    for (i = 0; i < N; i++) {
        // NOTE: passing &i is a race condition - all threads share the same variable i which changes each iteration.
        // Correct fix: define int ids[N], set ids[i] = i, and pass &ids[i] instead of &i.
        // The usleep reduces the risk but is not a guarantee.
        if (pthread_create(&philosophers[i], NULL, philosopher, (void*)&i) != 0) {
            fprintf(stderr, "Error by creating thread\n");
            return 2;
        }
        usleep(100000);
    }

    for (i = 0; i < N; i++) {
        if (pthread_join(philosophers[i], &result) != 0) {
            fprintf(stderr, "Error by joining thread\n");
            return 3;
        }
    }

    return 0;
}