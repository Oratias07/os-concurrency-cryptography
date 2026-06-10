#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define N 10
#define M 30

int matrix_threads[N][M];
void *fill_row(void *arg);
void *sum_of_rows(void *arg);
void create_threads(void*(func)(void *));
void print_matrix();
void calculate_sum_of_rows();

int main() {
    // create threads to fill the matrix with random numbers and print the matrix
    create_threads(fill_row);
    print_matrix();

    // create threads to calculate the sum of each row and print the total sum of all rows
    create_threads(sum_of_rows);
    calculate_sum_of_rows();
    return 0;
}

// create N threads 
void create_threads(void*(func)(void *)) {
    pthread_t threads[N];
    int indices[N];
    for (int i = 0; i < N; i++) {
        indices[i] = i;
        if (pthread_create(&threads[i], NULL, func, &indices[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }
    // wait for all threads to finish
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }
}

// function to fill a row of the matrix with random numbers
// the argument is the row number to fill
void *fill_row(void *arg) {
    int row = *(int *)arg;
    for (int j = 0; j < M; j++) {
        matrix_threads[row][j] = rand() % 101; // fill the row with random numbers between 0 and 100
    }
    return NULL; // return NULL to indicate the thread has finished its work
}

// function to calculate the sum of each row
void *sum_of_rows(void *arg) {
    int row = *(int *)arg;
    int sum = 0;
    for (int j = 0; j < M; j++) {
        sum += matrix_threads[row][j];
    }
    matrix_threads[row][M - 1] = sum; // store the sum in the last column
    return NULL; // return NULL to indicate the thread has finished its work
}

// function to calculate the total sum of all rows and print it
void calculate_sum_of_rows() {
    int total_sum = 0;
    for (int i = 0; i < N; i++) {
        total_sum += matrix_threads[i][M - 1]; // add the sum of each row to the total sum
    }
    printf("Total sum of all rows: %d\n", total_sum);
}

// function to print the filled matrix
void print_matrix() {
    // print the filled matrix
    printf("Filled Matrix:\n");
    printf("{\n");
    for (int i = 0; i < N; i++) {
        printf("  { ");
        for (int j = 0; j < M; j++) {
            printf("%d ", matrix_threads[i][j]);
            if (j < M - 1) {
                printf(", ");
            }
        }
        printf("}\n");
    }
    printf("}\n");
}