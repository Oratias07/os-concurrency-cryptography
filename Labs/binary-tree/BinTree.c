#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number>\n", argv[0]);
        return 1;
    }
    int mDepth = atoi(argv[1]) - 1;
    if (mDepth < 0) {
        printf("Please enter a non-negative integer for depth.\n");
        return 1;
    }
    pid_t pid = getpid();
    char buf[20];
    printf("Parent PID: %d\n", pid);
    if (mDepth > 0) {
        sprintf(buf, "%d", mDepth);
        pid_t left = fork();
        if (left == 0) {
            printf("Left child: Process PID: %d, Parent PID: %d\n", getpid(), pid);
            execlp("./BinTreeChild", "./BinTreeChild", buf, NULL);
            return 0;
        }
        pid_t right = fork();
        if (right == 0) {
            printf("Right child: Process PID: %d, Parent PID: %d\n", getpid(), pid);
            execlp("./BinTreeChild", "./BinTreeChild", buf, NULL);
            return 0;
        }
        wait(NULL);
        wait(NULL);
    }
    return 0;
}