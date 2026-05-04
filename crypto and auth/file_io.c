/*
 * file_io.c — File encryption/decryption with ChaCha20.
 *
 * Core loop: read 64-byte chunks, generate keystream via chacha20_block,
 * XOR, write. The same XOR routine handles both directions (stream ciphers
 * are symmetric). Encrypt and decrypt differ only in nonce handling.
 */

#include <stdio.h>
#include <errno.h>
#include "file_io.h"
#include "chacha20.h"

/* ---- Key derivation ----------------------------------------------------- */

void derive_key(const char *ascii_key, uint8_t key[KEY_SIZE]) {
    size_t n = strlen(ascii_key);
    for (size_t i = 0; i < KEY_SIZE; i++) {
        key[i] = (uint8_t)ascii_key[i % n];
    }
}

/* ---- Random nonce generation -------------------------------------------- */

/* Reads `len` cryptographic-quality random bytes from /dev/urandom. */
static int fill_random(uint8_t *out, size_t len) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return 0;
    size_t got = fread(out, 1, len, f);
    fclose(f);
    return got == len;
}

/* ---- XOR-with-keystream loop -------------------------------------------- */

/* Reads `fin` in 64-byte blocks, XORs each block with ChaCha20 keystream,
 * and writes the result to `fout`. Used by both encrypt and decrypt. */
static int chacha20_xor_stream(const uint8_t key[KEY_SIZE],
                               const uint8_t nonce[NONCE_SIZE],
                               FILE *fin, FILE *fout) {
    uint8_t keystream[CHACHA_BLOCK];
    uint8_t buffer[CHACHA_BLOCK];

    for (uint32_t counter = 0; ; counter++) {
        size_t n = fread(buffer, 1, CHACHA_BLOCK, fin);
        if (n == 0) {
            if (ferror(fin)) return 0;
            break;
        }
        chacha20_block(key, nonce, counter, keystream);
        for (size_t i = 0; i < n; i++) buffer[i] ^= keystream[i];
        if (fwrite(buffer, 1, n, fout) != n) return 0;
    }

    explicit_bzero(keystream, sizeof(keystream));
    explicit_bzero(buffer,    sizeof(buffer));
    return 1;
}

/* ---- Public encrypt/decrypt --------------------------------------------- */

int encrypt_file(const uint8_t key[KEY_SIZE],
                 const char *in_path, const char *out_path) {
    uint8_t nonce[NONCE_SIZE];
    if (!fill_random(nonce, NONCE_SIZE)) {
        fprintf(stderr, "failed to read from /dev/urandom: %s\n", strerror(errno));
        return 0;
    }

    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        fprintf(stderr, "open %s: %s\n", in_path, strerror(errno));
        return 0;
    }

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        fprintf(stderr, "open %s: %s\n", out_path, strerror(errno));
        fclose(fin);
        return 0;
    }

    int ok = 0;
    if (fwrite(nonce, 1, NONCE_SIZE, fout) == NONCE_SIZE) {
        ok = chacha20_xor_stream(key, nonce, fin, fout);
    }

    fclose(fin);
    fclose(fout);
    return ok;
}

int decrypt_file(const uint8_t key[KEY_SIZE],
                 const char *in_path, const char *out_path) {
    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        fprintf(stderr, "open %s: %s\n", in_path, strerror(errno));
        return 0;
    }

    uint8_t nonce[NONCE_SIZE];
    if (fread(nonce, 1, NONCE_SIZE, fin) != NONCE_SIZE) {
        fprintf(stderr, "input file is too short to contain a nonce\n");
        fclose(fin);
        return 0;
    }

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        fprintf(stderr, "open %s: %s\n", out_path, strerror(errno));
        fclose(fin);
        return 0;
    }

    int ok = chacha20_xor_stream(key, nonce, fin, fout);
    fclose(fin);
    fclose(fout);
    return ok;
}
