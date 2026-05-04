/*
 * file_io.h — File encryption and decryption using ChaCha20.
 *
 * Handles key derivation (ASCII → 32 bytes), nonce management, and the
 * read-XOR-write loop. Encrypt prepends a random nonce to the output;
 * decrypt reads the nonce back from the input header.
 *
 * Encrypted file layout:  [ 12-byte nonce ] [ ciphertext ... ]
 */

#ifndef FILE_IO_H
#define FILE_IO_H

#include "common.h"

/*
 * Stretches an ASCII key to 32 bytes by cycling its bytes.
 * NOTE: This is NOT a secure KDF — see README.md for details.
 */
void derive_key(const char *ascii_key, uint8_t key[KEY_SIZE]);

/*
 * Encrypts `in_path` → `out_path`.
 * Generates a fresh 12-byte random nonce and writes it as the file header.
 * Returns 1 on success, 0 on failure.
 */
int encrypt_file(const uint8_t key[KEY_SIZE],
                 const char *in_path, const char *out_path);

/*
 * Decrypts `in_path` → `out_path`.
 * Reads the 12-byte nonce from the file header, then XORs with keystream.
 * Returns 1 on success, 0 on failure.
 */
int decrypt_file(const uint8_t key[KEY_SIZE],
                 const char *in_path, const char *out_path);

#endif /* FILE_IO_H */
