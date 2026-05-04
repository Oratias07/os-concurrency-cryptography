/*
 * chacha20.c — ChaCha20 stream cipher (RFC 8439).
 *
 * The state is a 4×4 matrix of uint32 words:
 *     [ constant  constant  constant  constant ]
 *     [ key       key       key       key      ]
 *     [ key       key       key       key      ]
 *     [ counter   nonce     nonce     nonce    ]
 *
 * 20 rounds (10 iterations of column-round + diagonal-round) mix the state,
 * then the original state is added back in to make the function non-invertible.
 * The result is 64 bytes of pseudorandom keystream.
 */

#include "chacha20.h"

/* ARX (Add-Rotate-XOR) quarter-round on four state words.
 * The rotation constants 16, 12, 8, 7 maximize diffusion. */
static void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *a += *b; *d ^= *a; *d = rotl32(*d, 16);
    *c += *d; *b ^= *c; *b = rotl32(*b, 12);
    *a += *b; *d ^= *a; *d = rotl32(*d,  8);
    *c += *d; *b ^= *c; *b = rotl32(*b,  7);
}

void chacha20_block(const uint8_t key[KEY_SIZE],
                    const uint8_t nonce[NONCE_SIZE],
                    uint32_t counter,
                    uint8_t out[CHACHA_BLOCK]) {
    /* Build the initial 16-word state. */
    uint32_t state[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,   /* "expand 32-byte k" */
        load32_le(key +  0), load32_le(key +  4), load32_le(key +  8), load32_le(key + 12),
        load32_le(key + 16), load32_le(key + 20), load32_le(key + 24), load32_le(key + 28),
        counter,
        load32_le(nonce + 0), load32_le(nonce + 4), load32_le(nonce + 8)
    };

    /* Working copy — the original is needed for the final addition. */
    uint32_t ws[16];
    memcpy(ws, state, sizeof(state));

    /* 10 double-rounds = 20 rounds total. */
    for (int i = 0; i < 10; i++) {
        /* Column round */
        quarter_round(&ws[0], &ws[4], &ws[ 8], &ws[12]);
        quarter_round(&ws[1], &ws[5], &ws[ 9], &ws[13]);
        quarter_round(&ws[2], &ws[6], &ws[10], &ws[14]);
        quarter_round(&ws[3], &ws[7], &ws[11], &ws[15]);
        /* Diagonal round */
        quarter_round(&ws[0], &ws[5], &ws[10], &ws[15]);
        quarter_round(&ws[1], &ws[6], &ws[11], &ws[12]);
        quarter_round(&ws[2], &ws[7], &ws[ 8], &ws[13]);
        quarter_round(&ws[3], &ws[4], &ws[ 9], &ws[14]);
    }

    /* Add original state and serialize as little-endian bytes. */
    for (int i = 0; i < 16; i++) ws[i] += state[i];
    for (int i = 0; i < 16; i++) {
        out[i * 4 + 0] = (uint8_t)(ws[i]      );
        out[i * 4 + 1] = (uint8_t)(ws[i] >>  8);
        out[i * 4 + 2] = (uint8_t)(ws[i] >> 16);
        out[i * 4 + 3] = (uint8_t)(ws[i] >> 24);
    }
}
