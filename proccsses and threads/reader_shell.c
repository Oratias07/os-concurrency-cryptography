#include <stdio.h>      // printf, perror
#include <string.h>     // strcmp
#include <unistd.h>     // fork, execl
#include <sys/wait.h>   // wait

// Main function for the Reader Shell
// This shell allows users to execute commands related to reading room activities
// The shell continuously prompts the user for input until they choose to exit with the "Esc" command
// Supported commands:
// - date: Displays the current date and time using the date command
// - whoami: Displays the current user using the whoami command
// - pwd: Displays the current working directory using the pwd command
// - uptime: Displays the system uptime using the uptime command
// If an unsupported command is entered, the shell will print "Not Supported" and prompt the user again
// Note: The commands are executed in child processes created with fork() and the shell waits for the child process to finish before prompting the user again
// Example usage:
        // $ ./reader_shell
        // Reader$** date
        // [Output of date command]
        // Reader$** whoami
        // [Output of whoami command]
        // Reader$** pwd
        // [Output of pwd command]
        // Reader$** uptime
        // [Output of uptime command]
        // Reader$** Esc
        // Returning to the LibShell...
int main() {
    while (1) {
        char command[100];
        printf("Reader$** ");
        if (fgets(command, sizeof(command), stdin) == NULL) break;
        command[strcspn(command, "\n")] = 0;

        int did_fork = 0;

        if (strcmp(command, "Esc") == 0) {
            printf("Returning to the LibShell...\n");
            break;
        } else if (strcmp(command, "date") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                execl("/bin/date", "date", NULL);
                perror("File not found");
                return 1;
            }
            did_fork = 1;
        } else if (strcmp(command, "whoami") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                execl("/usr/bin/whoami", "whoami", NULL);
                perror("File not found");
                return 1;
            }
            did_fork = 1;
        } else if (strcmp(command, "pwd") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                execl("/bin/pwd", "pwd", NULL);
                perror("File not found");
                return 1;
            }
            did_fork = 1;
        } else if (strcmp(command, "uptime") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                execl("/usr/bin/uptime", "uptime", NULL);
                perror("File not found");
                return 1;
            }
            did_fork = 1;
        } else {
            printf("Not Supported\n");
        }
        if (did_fork) wait(NULL);
    }
}
