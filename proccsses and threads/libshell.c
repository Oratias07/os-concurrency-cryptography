#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit
#include <string.h>     // strcmp
#include <unistd.h>     // fork, execl
#include <sys/wait.h>   // wait

// Main function for the LibShell
// This shell allows users to enter different sub-shells (reader, catalog, archive) or exit the shell
// The shell continuously prompts the user for input until they choose to exit
// The shell uses fork() to create child processes for each sub-shell and waits for them to finish before prompting the user again
// Supported commands:
// - reader: Enters the Reader Shell for reading room commands
// - catalog: Enters the Catalog Shell for catalog search commands
// - archive: Enters the Archive Shell for file archive tools
// - exit: Exits the LibShell
// If an unsupported command is entered, the shell will print "Not Supported" and prompt the user again
// Note: The sub-shells (reader_shell, catalog_shell, archive_shell) must be implemented separately and should be executable in the same directory as this LibShell for the execl() calls to work correctly
// Example usage:
        // $ ./libshell
        // Welcome to LibShell!
        // Enter <reader> for reading room commands
        // Enter <catalog> for catalog search
        // Enter <archive> for file archive tools
        // Lib$** reader
        // Entering Reader Shell...
        // Reader$** date
        // [Output of date command]
        // Reader$** Esc
        // Returning to the LibShell...
        // Lib$** catalog
        // Entering Catalog Shell...
        // Catalog$** run wc book.txt
        // [Output of wc command on book.txt]
        // Catalog$** Esc
        // Returning to the LibShell...
        // Lib$** archive
        // Entering Archive Shell...
        // Archive$** count master.txt
        // [Output of wc command on master.txt]
        // Archive$** Esc
        // Returning to the LibShell...
        // Lib$** exit
        // Goodbye from Libshell
int main() {
    char command[100];

    printf("Welcome to LibShell!\n");
    printf("Enter <reader> for reading room commands\nEnter <catalog> for catalog search\nEnter <archive> for file archive tools\n");

    while (1) {
        printf("Lib$** ");
        if (fgets(command, sizeof(command), stdin) == NULL) break;
        command[strcspn(command, "\n")] = 0;

        int did_fork = 0;

        if (strcmp(command, "exit") == 0) {
            printf("Goodbye from Libshell\n");
            break;
        } else if (strcmp(command, "reader") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                printf("Entering Reader Shell...\n");
                execl("./reader_shell", "reader_shell", NULL);
                perror("Failed to execute reader");
                exit(EXIT_FAILURE);
            }
            did_fork = 1;
        } else if (strcmp(command, "catalog") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                printf("Entering Catalog Shell...\n");
                execl("./catalog_shell", "catalog_shell", NULL);
                perror("Failed to execute catalog");
                exit(EXIT_FAILURE);
            }
            did_fork = 1;
        } else if (strcmp(command, "archive") == 0) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); continue; }
            if (pid == 0) {
                printf("Entering Archive Shell...\n");
                execl("./archive_shell", "archive_shell", NULL);
                perror("Failed to execute archive");
                exit(EXIT_FAILURE);
            }
            did_fork = 1;
        } else {
            printf("Not Supported\n");
        }
        if (did_fork) wait(NULL);
    }
    return 0;
}
