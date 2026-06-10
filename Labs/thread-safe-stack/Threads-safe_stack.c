#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define K 5   /* Number of threads */
#define N 10  /* Stack capacity */

int stk[N];   /* Shared stack */
int idx = 0;  /* Stack top index */

sem_t isFull;         /* Counts occupied slots (0..N) */
sem_t isEmpty;        /* Counts free slots (0..N) */
pthread_mutex_t mutex; /* Protects idx and stk */

void stkPush(int num);
int stkPop();
void* tFunc(void* p);

int main(int argc, char* argv[]) {
    pthread_t tArr[K];
    int i = 0, ans[K];

    /* Initialize: N free slots, 0 full slots */
    sem_init(&isEmpty, 0, N);
    sem_init(&isFull, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    /* Spawn K threads, each randomly pushing and popping */
    for (i = 0; i < K; i++)
        ans[i] = pthread_create(&tArr[i], NULL, tFunc, NULL);

    sleep(10);
    return 0;
}

/* Thread function: randomly pushes or pops every second */
void* tFunc(void* p) {
    int num;
    while (1) {
        num = rand() % 100;
        if (rand() % 2)
            stkPush(num);
        else
            stkPop();
        sleep(1);
    }
}

/*
 * stkPush - pushes num onto the shared stack.
 * Blocks if the stack is full until a slot is freed by a pop.
 */
void stkPush(int num) {
    sem_wait(&isEmpty);          /* Wait for a free slot */
    pthread_mutex_lock(&mutex);
    stk[idx++] = num;
    printf("PUSH %d\n", num);
    pthread_mutex_unlock(&mutex);
    sem_post(&isFull);           /* Signal that a new item is available */
}

/*
 * stkPop - pops and returns the top value from the shared stack.
 * Blocks if the stack is empty until an item is pushed.
 */
int stkPop() {
    sem_wait(&isFull);           /* Wait for an available item */
    pthread_mutex_lock(&mutex);
    int val = stk[--idx];
    printf("POP %d\n", val);
    pthread_mutex_unlock(&mutex);
    sem_post(&isEmpty);          /* Signal that a slot was freed */
    return val;
}