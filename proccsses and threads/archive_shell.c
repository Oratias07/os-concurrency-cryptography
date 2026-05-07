#include <stdio.h>    // printf, perror
#include <string.h>   // strcmp
#include <fcntl.h>    // open
#include <unistd.h>   // fork, read, write, close, execl, unlink
#include <sys/wait.h> // wait
#include <stdlib.h>   // strtol
#include <sys/stat.h> // chmod

// Main function for the Archive Shell
// This shell allows users to execute commands related to file archiving and management
// The shell continuously prompts the user for input until they choose to exit with the "Esc" command
// Supported commands:
// - merge <src> <dst>: Merges the contents of the source file into the destination file using file descriptors and system calls for efficient file handling
// - count <filename>: Counts the number of lines, words, and characters in a file using the wc command
// - remove <filename>: Removes a specified file using the unlink system call
// - protect <mode> <filename>: Changes the permissions of a specified file using the chmod system call, where mode is an octal number representing the desired permissions (e.g., 755)
// If an unsupported command is entered, the shell will print "Not Supported" and prompt the user again
// Note: The commands are executed in child processes created with fork() and the shell waits for the child process to finish before prompting the user again
// Example usage:
        // $ ./archive_shell
        // Archive$** merge source.txt destination.txt
        // [Contents of source.txt are appended to destination.txt]
        // Archive$** count master.txt
        // [Output of wc command on master.txt]
        // Archive$** remove oldfile.txt
        // [oldfile.txt is removed from the directory]
        // Archive$** protect 755 important.txt
        // [Permissions of important.txt are changed to rwxr-xr-x]
        // Archive$** Esc
        // Returning to the LibShell...
int main() {
    while (1) {
        char command[100];
        printf("Archive$** ");
        int r = scanf("%s", command);
        if (r == EOF) break;
        if (r != 1) {
            printf("Missing parameters\n");
            continue;
        }

        int did_fork = 0;

        if (strcmp(command, "Esc") == 0) {
            printf("Returning to the LibShell...\n");
            break;
        } else if (strcmp(command, "merge") == 0) {
            char src[100], dst[100];
            r = scanf("%s %s", src, dst);
            if (r == EOF) break;
            if (r != 2) {
                printf("Missing parameters\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                int src_fd = open(src, O_RDONLY);
                int dst_fd = open(dst, O_WRONLY | O_APPEND, 0644);
                if (src_fd < 0) {
                    perror("File not found");
                    return 1;
                }
                if (dst_fd < 0) {
                    close(src_fd);
                    perror("File not found");
                    return 1;
                }

                char buffer[1024];
                ssize_t bytesRead;
                while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0) {
                    if (write(dst_fd, buffer, bytesRead) != bytesRead) {
                        perror("Failed to write to destination file");
                        return 1;
                    }
                }
                if (bytesRead < 0) {
                    perror("Failed to read from source file");
                    return 1;
                }
                close(src_fd);
                close(dst_fd);
                return 0;
            }
            did_fork = 1;
        } else if (strcmp(command, "count") == 0) {
            char fileName[100];
            r = scanf("%s", fileName);
            if (r == EOF) break;
            if (r != 1) {
                printf("Missing parameters\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                execl("/bin/wc", "wc", fileName, NULL);
                perror("File not found");
                return 1;
            }
            did_fork = 1;
        } else if (strcmp(command, "remove") == 0) {
            char fileName[100];
            r = scanf("%s", fileName);
            if (r == EOF) break;
            if (r != 1) {
                printf("Missing parameters\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                if (unlink(fileName) < 0) {
                    perror("File not found");
                    return 1;
                }
                return 0;
            }
            did_fork = 1;
        } else if (strcmp(command, "protect") == 0) {
            char mode[100], fileName[100];
            r = scanf("%s %s", mode, fileName);
            if (r == EOF) break;
            if (r != 2) {
                printf("Missing parameters\n");
                continue;
            }
            int modeInt = (int)strtol(mode, NULL, 8);
            if (modeInt < 0 || modeInt > 0777) {
                printf("Invalid Mode!!\n");
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                if (chmod(fileName, modeInt) < 0) {
                    perror("File not found");
                    return 1;
                }
                printf("Permissions updated\n");
                return 0;
            }
            did_fork = 1;
        } else {
            printf("Not Supported\n");
        }
        if (did_fork) wait(NULL);
    }
}
