#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* Struct to pass both the counters array and its size to the print thread */
typedef struct {
    long int *arr;
    int size;
} CountersArgs;

void* counterPromotion(void* arg);
void* printThreads(void* arg);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <N>\n", argv[0]);
        return 1;
    }

    CountersArgs arg;
    arg.size = atoi(argv[1]);
    if (arg.size < 1) { printf("The N size is abnormal!\n"); return 1; }

    int i;
    pthread_t threads[arg.size + 1]; /* N counter threads + 1 print thread */

    /* Allocate N counters, initialized to 0 */
    long int *counters = calloc(arg.size, sizeof(long int));
    if (!counters) { perror("calloc"); return 1; }
    arg.arr = counters;

    /* Create N threads, each incrementing its own counter */
    for (i = 0; i < arg.size; i++) {
        if (pthread_create(&threads[i], NULL, counterPromotion, &arg.arr[i]) != 0) {
            perror("Failed to create thread");
            free(counters);
            exit(EXIT_FAILURE);
        }
    }

    /* Create the print thread — passes the full struct (array + size) */
    if (pthread_create(&threads[i], NULL, printThreads, &arg) != 0) {
        perror("Failed to create thread");
        free(counters);
        exit(EXIT_FAILURE);
    }

    /* Run for 20 seconds, then exit — return from main calls exit(),
       which terminates all threads */
    sleep(20);
    free(counters);
    return 0;
}

/* Endlessly increments the counter it was assigned */
void* counterPromotion(void* arg) {
    while (1) {
        *(long int*)arg += 1;
    }
}

/* Every 2 seconds, prints all counter values */
void* printThreads(void* arg) {
    CountersArgs counter_arg = *(CountersArgs*)arg;
    int print_count = 0;

    while (1) {
        printf("--- Snapshot #%d ---\n", ++print_count);
        for (int i = 0; i < counter_arg.size; i++) {
            printf("  Counter[%d]: %ld\n", i, counter_arg.arr[i]);
        }
        printf("--------------------\n\n");
        sleep(2);
    }
}