#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

// This program creates an encrypted copy of a file by reading from the source file, writing the encrypted data to a new file in the "encrypted" directory, and appending a specified string after each chunk of encrypted data.
// Usage: ./createEncryptedCopy <source_file> <buffer_size> <string_to_append>

// The program output is a new file named "a.txt" in the "encrypted" directory, which contains the encrypted data from the source file followed by the specified string after each chunk of data.
// Example usage:
// a.txt content: "abcdefg yyz" 
// ./createEncryptedCopy a.txt 2 xx
// This will read from "a.txt" in chunks of 2 bytes, write the encrypted data to "encrypted/a.txt", and append "xx" after each chunk of encrypted data.
// the output filr will be "encrypted/a.txt" with content: "abxxcdxxefxxg xxyyxxzxx"
// this is different from the original output from the -exercise output- which has content: "abxxcdxxefxxg xxyyxxz"

int main(int argc, char *argv[]) {
    int from, to;
    int rbytes, wbytes;

    // Check for the correct number of arguments
    if(argc != 4) { printf("Usage: %s <source_file> <buffer_size> <string_to_append>\n", argv[0]); return 1; }

    int N = atoi(argv[2]); // Convert the buffer size argument to an integer
    char buffer[N]; // Buffer size based on the second argument

    // Open the source file for reading
    from = open(argv[1], O_RDONLY);
    if (from == -1) {
        perror("Error opening file for reading");
        return 2;
    }

    // Create the "encrypted" directory
    if (mkdir("encrypted", 0777) == -1) {
        perror("Error creating directory");
        close(from);
        exit(1);
    }

    // Open the destination file for writing
    to = open("encrypted/a.txt", O_WRONLY | O_CREAT, 0644);
    if (to == -1) {
        perror("Error opening file for writing");
        close(from);
        exit(3);
    }

    // Read from the source file and write to the destination file
    while ((rbytes = read(from, buffer, N)) > 0) {
        // Write the encrypted data to the destination file
        wbytes = write(to, buffer, rbytes);
        int strbytes = write(to, argv[3], strlen(argv[3])); // Write the STR after the encrypted data
        if ((wbytes != rbytes) || (strbytes != strlen(argv[3]))) {
            perror("Error writing to file");
            close(from);
            close(to);
            return 4;
        }
    }

    // Close the files
    close(from);
    close(to);

    return (EXIT_SUCCESS);
}
