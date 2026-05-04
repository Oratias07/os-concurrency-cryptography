#include <stdio.h>    // printf, perror
#include <string.h>   // strcmp
#include <fcntl.h>    // open
#include <unistd.h>   // fork, read, write, close, execl, unlink
#include <sys/wait.h> // wait
#include <stdlib.h>   // atoi
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
        if (scanf("%s", command) != 1) {
            printf("Missing parameters\n");
            continue; // EOF or error, prompt again
        }

        if (strcmp(command, "Esc") == 0) {
            printf("Returning to the LibShell...\n");
            break; // Exit the Archive shell
        } else if (strcmp(command, "merge") == 0) {
            char src[100], dst[100];
            // Check if both parameters (source and destination filenames) are provided for the merge command
            // The merge command appends the contents of the source file to the destination file
            if (scanf("%s %s", src, dst) != 2) {
                printf("Missing parameters\n");
                continue; // EOF or error, prompt again
            }
            if (fork() == 0) {
                int src_fd = open(src, O_RDONLY);
                int dst_fd = open(dst, O_WRONLY | O_APPEND, 0644);
                // Check if the source and destination files were opened successfully
                if (src_fd < 0) {
                    perror("File not found");
                    return 1; // Exit child process
                }
                if (dst_fd < 0) {
                    perror("File not found");
                    return 1; // Exit child process
                }

                char buffer[1024];
                size_t bytesRead;
                // Read from the source file and write to the destination file in chunks
                // This approach is efficient for handling large files without consuming excessive memory
                while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0) {
                    if (write(dst_fd, buffer, bytesRead) != bytesRead) {
                        perror("Failed to write to destination file");
                        return 1; // Exit child process
                    }
                }
                if (bytesRead < 0) {
                    perror("Failed to read from source file");
                    return 1; // Exit child process
                }
                close(src_fd); // Close the file descriptor after checking
                close(dst_fd); // Close the file descriptor after checking
                return 0; // Exit child process successfully
            }
        } else if (strcmp(command, "count") == 0) {
            char fileName[100];
            // Check if the filename parameter is provided for the count command
            if (scanf("%s", fileName) != 1) {
                printf("Missing parameters\n");
                continue; // EOF or error, prompt again
            }
            if (fork() == 0) {
                execl("/bin/wc", "wc", fileName, NULL);
                perror("File not found");
                return 1; // Exit child process
            }
        } else if (strcmp(command, "remove") == 0) {
            char fileName[100];
            // Check if the filename parameter is provided for the remove command
            if (scanf("%s", fileName) != 1) {
                printf("Missing parameters\n");
                continue; // EOF or error, prompt again
            }
            if (fork() == 0) {
                if (unlink(fileName) < 0) { // Remove the file using unlink system call
                    perror("File not found");
                    return 1; // Exit child process
                }
                return 0; // Exit child process successfully
            }
        } else if (strcmp(command, "protect") == 0) {
            char mode[100], fileName[100];
            // Check if both parameters (mode and filename) are provided for the protect command
            if (scanf("%s %s", mode, fileName) != 2) {
                printf("Missing parameters\n");
                continue; // EOF or error, prompt again
            }
            int modeInt = (int)strtol(mode, NULL, 8); // Convert the mode string to an integer using strtol with base 8 (octal)
            if (modeInt < 0 || modeInt > 0777) {
                printf("Invalid Mode!!\n");
                continue; // Invalid mode
            }
            if (fork() == 0) {
                if (chmod(fileName, modeInt) < 0) { // Change file permissions using chmod system call
                    perror("File not found");
                    return 1; // Exit child process
                }
                printf("Permissions updated\n");
                return 0; // Exit child process successfully
            }
        } else {
            printf("Not Supported\n");
        }
        wait(NULL); // Wait for the child process to finish
    }
}