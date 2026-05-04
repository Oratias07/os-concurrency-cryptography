/*
 * common.h — Shared constants and inline helpers for file_crypt.
 *
 * All modules include this header for the fundamental type definitions,
 * crypto constants, and bit-manipulation primitives.
 */

#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>
#include <strings.h>

/* ---- Crypto constants --------------------------------------------------- */

#define KEY_SIZE        32   /* ChaCha20 key length in bytes (RFC 8439)      */
#define NONCE_SIZE      12   /* ChaCha20 nonce length in bytes               */
#define CHACHA_BLOCK    64   /* Bytes of keystream produced per block        */
#define MIN_KEY_CHARS    8   /* Assignment: encryption key >= 8 ASCII chars  */
#define SHA256_DIGEST   32   /* SHA-256 output size in bytes                 */

/* ---- Inline bit helpers ------------------------------------------------- */

/* Little-endian load of 4 bytes into a uint32_t. */
static inline uint32_t load32_le(const uint8_t *p) {
    return  (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* Rotate left 32-bit — used by ChaCha20. */
static inline uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

/* Rotate right 32-bit — used by SHA-256. */
static inline uint32_t rotr32(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

#endif /* COMMON_H */
