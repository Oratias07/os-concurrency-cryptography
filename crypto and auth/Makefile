# Makefile for file_crypt
#
# Usage:
#   make            — build the program
#   make clean      — delete compiled objects and binary
#   make test       — build and run a quick round-trip test
#
# After building:
#   ./file_crypt <passcode> <input_file> <encryption_key> <0|1>
#   Example: ./file_crypt Cyber1234 secret.txt mykey12345 0

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra
TARGET  = file_crypt

SRCS    = main.c sha256.c auth.c chacha20.c file_io.c
OBJS    = $(SRCS:.c=.o)
HEADERS = common.h sha256.h auth.h chacha20.h file_io.h

# Link all object files into the final binary.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile each .c to .o — rebuild if any header changes.
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) encrypted_output.dat decrypted_output.dat

test: $(TARGET)
	@echo "Hello, this is a test file." > _test_input.txt
	@./$(TARGET) Cyber1234 _test_input.txt testkey123 0
	@./$(TARGET) Cyber1234 encrypted_output.dat testkey123 1
	@diff -q _test_input.txt decrypted_output.dat && echo "PASS: round-trip OK" || echo "FAIL"
	@rm -f _test_input.txt encrypted_output.dat decrypted_output.dat

.PHONY: clean test
