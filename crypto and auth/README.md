# file_crypt

ChaCha20 file encryptor/decryptor with SHA-256 passcode verification.

## Build & Run

```bash
make                    # build
make test               # build + round-trip test
make clean              # remove all build artifacts
```

```bash
./file_crypt <login_passcode> <input_file> <encryption_key> <0|1>

# Encrypt:
./file_crypt Cyber1234 secret.pdf mySecretKey99 0
# → writes encrypted_output.dat

# Decrypt:
./file_crypt Cyber1234 encrypted_output.dat mySecretKey99 1
# → writes decrypted_output.dat
```

## Project Structure

```
├── Makefile        Build rules (compile each .c → .o, then link)
├── README.md       This file
├── common.h        Shared constants (KEY_SIZE, NONCE_SIZE, ...) and
│                   inline bit-manipulation helpers (load32_le, rotl32, rotr32)
├── sha256.h/.c     SHA-256 hash function (FIPS 180-4)
├── auth.h/.c       Login passcode verification — stores only the SHA-256
│                   digest, never the passcode itself
├── chacha20.h/.c   ChaCha20 stream cipher block function (RFC 8439)
├── file_io.h/.c    Key derivation, nonce management, file encrypt/decrypt
└── main.c          CLI parsing, validation, dispatch
```

## Design Decisions

**Passcode storage**: The passcode "Cyber1234" is never stored as plaintext
or in encrypted form. The binary contains only its SHA-256 digest (32 raw
bytes). Verification hashes the user's input and compares digests in constant
time.

**Encryption algorithm**: ChaCha20 (RFC 8439). A symmetric stream cipher —
the same XOR-with-keystream operation encrypts and decrypts. The two modes
differ only in nonce handling (generate vs. read).

**File format**: `[12-byte random nonce][ciphertext]`. The nonce is generated
from `/dev/urandom` at encryption time and stored as the file header. Decryption
reads it back. Each encryption produces different ciphertext even with the same
key and plaintext.

**Key derivation**: The ASCII key is cycled to fill 32 bytes. This is a
placeholder, not a real KDF — see known weaknesses below.

## Known Weaknesses

These are documented intentionally and are outside the scope of the assignment:

1. **Weak KDF** — Repeating the ASCII key provides no protection against
   brute force. A real system needs PBKDF2-HMAC-SHA256 or Argon2id with a
   stored salt and high iteration count.

2. **No authentication (MAC)** — ChaCha20 alone does not detect ciphertext
   tampering. Flipping a bit in the ciphertext flips the corresponding
   plaintext bit silently. Production code must use ChaCha20-Poly1305 (AEAD)
   or Encrypt-then-MAC with HMAC-SHA256.

3. **argv exposure** — The passcode and encryption key are visible in
   `ps` and `/proc/<pid>/cmdline`. The assignment mandates CLI parameters,
   so this is unavoidable under the given interface.

4. **Low-entropy passcode digest** — "Cyber1234" can be recovered from its
   SHA-256 digest via dictionary attack in seconds. A per-installation salt
   plus a slow KDF would mitigate this.
