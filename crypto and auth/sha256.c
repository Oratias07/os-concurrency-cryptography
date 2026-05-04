/*
 * sha256.c — SHA-256 implementation (FIPS 180-4).
 *
 * Processes messages in 512-bit (64-byte) blocks. Each block goes through
 * 64 rounds of mixing via the sha256_transform compression function. The
 * final digest is 256 bits (32 bytes).
 */

#include "sha256.h"

/* 64 round constants: fractional parts of the cube roots of the first
 * 64 primes (2..311). Defined by the SHA-256 specification. */
static const uint32_t SHA256_K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/* Initializes the hash state with the standard IV (fractional parts of
 * the square roots of the first 8 primes). */
void sha256_init(sha256_ctx *c) {
    c->state[0] = 0x6a09e667; c->state[1] = 0xbb67ae85;
    c->state[2] = 0x3c6ef372; c->state[3] = 0xa54ff53a;
    c->state[4] = 0x510e527f; c->state[5] = 0x9b05688c;
    c->state[6] = 0x1f83d9ab; c->state[7] = 0x5be0cd19;
    c->bit_count = 0;
    c->buffer_len = 0;
}

/* Compression function: processes one 512-bit block and updates the state.
 * This is the core of SHA-256 — called once per 64 input bytes. */
static void sha256_transform(sha256_ctx *c, const uint8_t block[64]) {
    uint32_t w[64];

    /* Load the 16 message words from the block (big-endian). */
    for (int i = 0; i < 16; i++) {
        w[i] =  ((uint32_t)block[i * 4 + 0] << 24)
             |  ((uint32_t)block[i * 4 + 1] << 16)
             |  ((uint32_t)block[i * 4 + 2] <<  8)
             |   (uint32_t)block[i * 4 + 3];
    }

    /* Expand the 16 words into 64 using the sigma functions. */
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i - 15],  7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >>  3);
        uint32_t s1 = rotr32(w[i -  2], 17) ^ rotr32(w[i -  2], 19) ^ (w[i -  2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    /* Initialize working variables from current hash state. */
    uint32_t a = c->state[0], b = c->state[1], cc = c->state[2], d = c->state[3];
    uint32_t e = c->state[4], f = c->state[5], g  = c->state[6], h = c->state[7];

    /* 64 rounds of mixing. */
    for (int i = 0; i < 64; i++) {
        uint32_t S1   = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch   = (e & f) ^ (~e & g);
        uint32_t tmp1 = h + S1 + ch + SHA256_K[i] + w[i];
        uint32_t S0   = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj  = (a & b) ^ (a & cc) ^ (b & cc);
        uint32_t tmp2 = S0 + maj;

        h = g; g = f; f = e; e = d + tmp1;
        d = cc; cc = b; b = a; a = tmp1 + tmp2;
    }

    /* Feed-forward: add compressed values back into the running state.
     * This addition is what makes SHA-256 one-way. */
    c->state[0] += a; c->state[1] += b; c->state[2] += cc; c->state[3] += d;
    c->state[4] += e; c->state[5] += f; c->state[6] += g;  c->state[7] += h;
}

/* Feeds arbitrary-length data into the hash, processing full 64-byte
 * blocks as they accumulate. Leftover bytes stay in the internal buffer. */
void sha256_update(sha256_ctx *c, const uint8_t *data, size_t len) {
    c->bit_count += (uint64_t)len * 8;
    while (len > 0) {
        size_t take = 64 - c->buffer_len;
        if (take > len) take = len;
        memcpy(c->buffer + c->buffer_len, data, take);
        c->buffer_len += take;
        data += take;
        len  -= take;
        if (c->buffer_len == 64) {
            sha256_transform(c, c->buffer);
            c->buffer_len = 0;
        }
    }
}

/* Finalizes the hash: appends the padding (0x80, zeros, 64-bit length)
 * and writes the 32-byte digest to `out`. */
void sha256_final(sha256_ctx *c, uint8_t out[SHA256_DIGEST]) {
    c->buffer[c->buffer_len++] = 0x80;
    if (c->buffer_len > 56) {
        while (c->buffer_len < 64) c->buffer[c->buffer_len++] = 0;
        sha256_transform(c, c->buffer);
        c->buffer_len = 0;
    }
    while (c->buffer_len < 56) c->buffer[c->buffer_len++] = 0;

    /* Append total message length in bits, big-endian. */
    for (int i = 7; i >= 0; i--) {
        c->buffer[c->buffer_len++] = (uint8_t)(c->bit_count >> (i * 8));
    }
    sha256_transform(c, c->buffer);

    /* Serialize state to bytes, big-endian. */
    for (int i = 0; i < 8; i++) {
        out[i * 4 + 0] = (uint8_t)(c->state[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(c->state[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(c->state[i] >>  8);
        out[i * 4 + 3] = (uint8_t)(c->state[i]      );
    }
}

/* One-shot wrapper: hashes `len` bytes and writes the digest. */
void sha256(const uint8_t *data, size_t len, uint8_t out[SHA256_DIGEST]) {
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, data, len);
    sha256_final(&c, out);
}
