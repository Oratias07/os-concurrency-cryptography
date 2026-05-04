#!/bin/bash

# This script compiles the C programs for the library shell and its associated shells (reader, catalog, archive).
# It also checks for the presence of the necessary source files before attempting to compile them, ensuring that the compilation process does not fail due to missing files.
# After compiling, it makes the compiled programs executable and runs the main library shell.
# Usage: ./run_lib.sh
# Note: Ensure that you have the necessary permissions to execute this script and that you have a C compiler (like gcc) installed on your system.

# Check if the main shell source file exists
if [ ! -f "libshell.c" ]; then
    echo "Error: libshell.c not found!"
    exit 1
fi

# Check if the required C files exist before compiling
for file in reader_shell.c catalog_shell.c archive_shell.c; do
    if [ ! -f "$file" ]; then
        echo "Error: $file not found!"
        exit 1
    fi
done

# Compile the C program
gcc -o libshell libshell.c
gcc -o reader_shell reader_shell.c
gcc -o catalog_shell catalog_shell.c
gcc -o archive_shell archive_shell.c

# Check if the compilation was successful
if [ ! -f "libshell" ] || [ ! -f "reader_shell" ] || [ ! -f "catalog_shell" ] || [ ! -f "archive_shell" ]; then
    echo "Error: Compilation failed!"
    exit 1
fi

# Set permissions for the compiled programs
chmod +x libshell reader_shell catalog_shell archive_shell

# Run the compiled program
echo "Welcome to LibShell!"
./libshell