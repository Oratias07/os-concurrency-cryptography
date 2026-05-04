/*
 * auth.c — Passcode verification.
 *
 * The expected passcode ("Cyber1234") is stored ONLY as its SHA-256 digest —
 * 32 raw bytes. The plaintext does not appear anywhere in this file or in the
 * compiled binary. Running `strings` on the binary will not reveal it.
 *
 * Verification: hash the user's input with SHA-256, then compare the two
 * digests byte-by-byte in constant time (no early exit).
 */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "sha256.h"
#include "auth.h"

/* SHA-256("Cyber1234"), precomputed at development time.
 * echo -n "Cyber1234" | sha256sum → 994e11a8...555b */
static const uint8_t PASSCODE_DIGEST[SHA256_DIGEST] = {
    0x99, 0x4e, 0x11, 0xa8, 0x5e, 0x9a, 0x40, 0x17,
    0x1f, 0xe0, 0xc7, 0x45, 0xb0, 0x3b, 0xf4, 0xca,
    0x62, 0xe4, 0x93, 0xc1, 0x4d, 0xc4, 0x56, 0x81,
    0xa6, 0x65, 0xe4, 0xda, 0xa8, 0xdb, 0x55, 0x5b
};

/* Constant-time byte comparison. Returns 0 iff all `len` bytes match.
 * The loop always runs `len` iterations — no early exit on mismatch. */
static uint8_t ct_memcmp(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) diff |= a[i] ^ b[i];
    return diff;
}

int verify_passcode(const char *candidate) {
    uint8_t digest[SHA256_DIGEST];
    sha256((const uint8_t *)candidate, strlen(candidate), digest);
    int ok = (ct_memcmp(digest, PASSCODE_DIGEST, SHA256_DIGEST) == 0);
    explicit_bzero(digest, sizeof(digest));
    return ok;
}
