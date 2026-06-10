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
    pid_t pid = getpid();
    pid_t left_child_pid, right_child_pid;
    char buf[20];

    left_child_pid = fork();
    if (left_child_pid < 0) {
        perror("Fork failed");
        return 1;
    } else if (left_child_pid == 0) {
        printf("Left child: Process PID: %d, Parent PID: %d\n", getpid(), pid);
        mDepth--;
        if (mDepth > 0) {
            sprintf(buf, "%d", mDepth);
            execlp("./BinTreeChild", "./BinTreeChild", buf, NULL);
        }
        return 0;
    }

    right_child_pid = fork();
    if (right_child_pid < 0) {
        perror("Fork failed");
        return 1;
    } else if (right_child_pid == 0) {
        printf("Right child: Process PID: %d, Parent PID: %d\n", getpid(), pid);
        mDepth--;
        if (mDepth > 0) {
            sprintf(buf, "%d", mDepth);
            execlp("./BinTreeChild", "./BinTreeChild", buf, NULL);
        }
        return 0;
    }

    wait(NULL);
    wait(NULL);
    return 0;
}