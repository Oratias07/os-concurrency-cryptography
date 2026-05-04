/*
 * sha256.h — SHA-256 hash function (FIPS 180-4).
 *
 * Standalone implementation with no external dependencies beyond common.h.
 * Used here to hash the login passcode for verification without storing
 * the passcode itself.
 */

#ifndef SHA256_H
#define SHA256_H

#include "common.h"

/* Opaque context for incremental hashing (init → update → final). */
typedef struct {
    uint32_t state[8];      /* Current hash state (H0..H7)                   */
    uint64_t bit_count;     /* Total number of message bits seen so far      */
    uint8_t  buffer[64];    /* Partial-block buffer                          */
    size_t   buffer_len;    /* Bytes currently in buffer (0..63)             */
} sha256_ctx;

/* Incremental API */
void sha256_init     (sha256_ctx *c);
void sha256_update   (sha256_ctx *c, const uint8_t *data, size_t len);
void sha256_final    (sha256_ctx *c, uint8_t out[SHA256_DIGEST]);

/* One-shot convenience wrapper */
void sha256(const uint8_t *data, size_t len, uint8_t out[SHA256_DIGEST]);

#endif /* SHA256_H */
