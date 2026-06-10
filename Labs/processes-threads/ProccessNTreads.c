#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#define N 10

// This program creates N-1 child processes to calculate the Fibonacci sequence up to the Nth term. Each child process calculates a term in the sequence and prints it, while the parent process waits for all child processes to finish before printing the final term.
// The Fibonacci sequence is defined as follows:
// F(0) = 0
// F(1) = 1
// F(n) = F(n-1) + F(n-2) for n > 1
int main() {
    int c1 = 0, c2 = 1, t, i;

    for (i = 1; i < N; i++) {
        switch (fork()) {
            case -1: // Error handling for fork failure
                printf("Error creating process\n");
                return -1;
            case 0: // Child process calculates the Fibonacci term
                for (int j = 0; j < N - i; j++) { // Calculate the Fibonacci term for the current child process
                    t = c1 + c2;
                    c1 = c2;
                    c2 = t;
                }
                printf("%d ", c2);
                return 0;
            default: // Parent process waits for the child process to finish
                wait(NULL);
        }
    }

    printf("%d\n", c2); // Print the final term of the Fibonacci sequence after all child processes have completed
    return 0;
}