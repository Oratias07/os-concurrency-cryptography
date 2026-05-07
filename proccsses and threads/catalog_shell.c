#include <stdio.h>     // printf, perror
#include <string.h>    // strcmp
#include <unistd.h>    // fork, execl
#include <sys/wait.h>  // wait
#include <stdlib.h>

// Main function for the Catalog Shell
// This shell allows users to execute commands related to catalog searching and file management
// The shell continuously prompts the user for input until they choose to exit with the "Esc" command
// Supported commands:
// - find <word> <filename>: Searches for a specific word in a file using the grep command
// - ls -l: Lists the files in the current directory in long format using the ls command with the -l option
// - newcat <filename>: Creates a new directory with the specified name using the mkdir command
// - run <program> <argument>: Executes a specified program with an argument using execlp, allowing for more flexible command execution
// If an unsupported command is entered, the shell will print "Not Supported" and prompt the user again
// Note: The commands are executed in child processes created with fork() and the shell waits for the child process to finish before prompting the user again
// Example usage:
        // $ ./catalog_shell
        // Catalog$** find library book.txt
        // [Output of grep command searching for "library" in book.txt]
        // Catalog$** ls -l
        // [Output of ls command in long format]
        // Catalog$** newcat new_directory
        // [A new directory named "new_directory" is created]
        // Catalog$** run wc book.txt
        // [Output of wc command on book.txt]
        // Catalog$** Esc
        // Returning to the LibShell...
int main() {
    while (1) {
        char command[100];
        printf("Catalog$** ");
        int r = scanf("%s", command);
        if (r == EOF) break;
        if (r != 1) {
            printf("Missing parameters\n");
            break;
        }

        int did_fork = 0;

        if (strcmp(command, "Esc") == 0) {
            printf("Returning to the LibShell...\n");
            break;
        } else if (strcmp(command, "find") == 0) {
            char line[200];
            char word[100], fileName[100];
            if (fgets(line, sizeof(line), stdin) == NULL) break;

            if (sscanf(line, "%s %s", word, fileName) != 2) {
                printf("Missing parameters\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                execl("/bin/grep", "grep", word, fileName, NULL);
                perror("File not found");
                return 1;
            }
            did_fork = 1;
        } else if (strcmp(command, "ls") == 0) {
            char line[200];
            char arg[100];

            if (fgets(line, sizeof(line), stdin) == NULL) break;

            if (sscanf(line, "%s", arg) != 1) {
                printf("Missing parameters\n");
                continue;
            } else if (strcmp(arg, "-l") != 0) {
                printf("Not Supported\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                execl("/bin/ls", "ls", "-l", NULL);
                perror("execl failed");
                exit(1);
            }
            did_fork = 1;
        } else if (strcmp(command, "newcat") == 0) {
            char line[200];
            char fileName[100];

            if (fgets(line, sizeof(line), stdin) == NULL) break;

            if (sscanf(line, "%s", fileName) != 1) {
                printf("Missing parameters\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                execl("/bin/mkdir", "mkdir", fileName, NULL);
                perror("File not found");
                exit(1);
            }
            did_fork = 1;
        } else if (strcmp(command, "run") == 0) {
            char line[200];
            char prog[100], arg[100];

            if (fgets(line, sizeof(line), stdin) == NULL) break;

            if (sscanf(line, "%s %s", prog, arg) != 2) {
                printf("Missing parameters\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                execlp(prog, prog, arg, NULL);
                perror("File not found");
                exit(1);
            }
            did_fork = 1;
        } else {
            printf("Not Supported\n");
        }
        if (did_fork) wait(NULL);
    }
}
