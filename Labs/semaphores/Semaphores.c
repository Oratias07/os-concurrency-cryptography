#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
// Define the number of threads
#define N 3
// Declare semaphores
sem_t s1, s2, s3;

// Function prototypes for thread functions
void* A(void*);
void* B(void*);
void* C(void*);

// Creates N threads that print 'A', 'B', 'C', 'C' repeatedly using semaphores for synchronization.
int main() {
    pthread_t threads[N]; // Array to hold thread identifiers

    void* (*printChar)(void*); // Function pointer to hold the thread function
    char ch[3] = {'A', 'B', 'C'}; // Array to hold characters for each thread

    // Initialize semaphores
    sem_init(&s1, 0, 1);
    sem_init(&s2, 0, 0);
    sem_init(&s3, 0, 0);

    for (int i = 0; i < N; i++) {
        // Assign the appropriate thread function based on the index
        if ((i%3) == 0) printChar = A;
        if ((i%3) == 1) printChar = B;
        if ((i%3) == 2) printChar = C;
        
        if (pthread_create(&threads[i], NULL, printChar, &ch[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    sleep(5); // Allow threads to run for a while before terminating the program
    return 0;
}

// Thread function for thread A
void* A(void* ch) {
    while (1) { // Infinite loop to keep the thread running
        sem_wait(&s1); // Wait on semaphore s1
        printf("%c\n", *(char*)ch); // Print the character associated with this thread (A)
        sem_post(&s2); // Signal semaphore s2 to allow thread B to run
    }
}
// Thread function for thread B
void* B(void* ch) {
    while (1) {
        sem_wait(&s2);
        printf("%c\n", *(char*)ch);
        sem_post(&s3);
        sem_post(&s3);
    }
}
// Thread function for thread C
void* C(void* ch) {
    int flag = 0;
    while (1) {
        sem_wait(&s3);
        flag++;
        printf("%c\n", *(char*)ch);
        if (flag%2 == 0) sem_post(&s1);     
    }
}