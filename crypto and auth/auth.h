/*
 * auth.h — Passcode verification.
 *
 * Verifies the user-supplied login passcode against a precomputed SHA-256
 * digest. The plaintext passcode never appears in the source or binary.
 * Comparison is constant-time to prevent timing side-channels.
 */

#ifndef AUTH_H
#define AUTH_H

/*
 * Returns 1 if `candidate` matches the expected passcode, 0 otherwise.
 * Internally hashes `candidate` with SHA-256 and compares against the
 * embedded digest in constant time.
 */
int verify_passcode(const char *candidate);

#endif /* AUTH_H */
