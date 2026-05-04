/*
 * main.c — Entry point for file_crypt.
 *
 * Parses the 4 CLI parameters, verifies the login passcode, derives the
 * encryption key, and dispatches to encrypt_file or decrypt_file.
 *
 * Usage:
 *     ./file_crypt <login_passcode> <input_file> <encryption_key> <0|1>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "auth.h"
#include "file_io.h"

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s <login_passcode> <input_file> <encryption_key> <0|1>\n"
        "  0 = encrypt  ->  encrypted_output.dat\n"
        "  1 = decrypt  ->  decrypted_output.dat\n"
        "  <encryption_key> must be at least %d ASCII characters.\n",
        prog, MIN_KEY_CHARS);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage(argv[0]);
        return 1;
    }

    const char *login_passcode = argv[1];
    const char *input_path     = argv[2];
    const char *encryption_key = argv[3];
    int         action_flag    = atoi(argv[4]);

    /* Validate constraints before any crypto work. */
    if (strlen(encryption_key) < MIN_KEY_CHARS) {
        fprintf(stderr, "encryption key must be at least %d characters\n", MIN_KEY_CHARS);
        return 1;
    }
    if (action_flag != 0 && action_flag != 1) {
        fprintf(stderr, "action flag must be 0 (encrypt) or 1 (decrypt)\n");
        return 1;
    }

    /* Login gate — wrong passcode → nonzero exit. */
    if (!verify_passcode(login_passcode)) {
        fprintf(stderr, "Incorrect passcode. Access denied.\n");
        return 1;
    }

    /* Derive the 32-byte ChaCha20 key from the ASCII key. */
    uint8_t key[KEY_SIZE];
    derive_key(encryption_key, key);

    /* Dispatch. */
    const char *out_path = (action_flag == 0) ? "encrypted_output.dat"
                                              : "decrypted_output.dat";
    int ok;
    if (action_flag == 0) {
        fprintf(stderr, "Encrypting %s -> %s\n", input_path, out_path);
        ok = encrypt_file(key, input_path, out_path);
    } else {
        fprintf(stderr, "Decrypting %s -> %s\n", input_path, out_path);
        ok = decrypt_file(key, input_path, out_path);
    }

    /* Wipe key material before exit. */
    explicit_bzero(key, sizeof(key));
    return ok ? 0 : 1;
}
