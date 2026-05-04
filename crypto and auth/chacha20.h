/*
 * chacha20.h — ChaCha20 stream cipher (RFC 8439).
 *
 * Exposes the block function that produces 64 bytes of keystream per call.
 * The higher-level encrypt/decrypt logic lives in file_io.c.
 */

#ifndef CHACHA20_H
#define CHACHA20_H

#include "common.h"

/*
 * Generates one 64-byte keystream block for the given (key, nonce, counter).
 *
 *   key     — 32-byte encryption key
 *   nonce   — 12-byte nonce (must be unique per key)
 *   counter — block counter (0 for the first 64 bytes, 1 for the next, ...)
 *   out     — 64-byte output buffer for the keystream
 */
void chacha20_block(const uint8_t key[KEY_SIZE],
                    const uint8_t nonce[NONCE_SIZE],
                    uint32_t counter,
                    uint8_t out[CHACHA_BLOCK]);

#endif /* CHACHA20_H */
