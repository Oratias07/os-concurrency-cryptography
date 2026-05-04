# Operating Systems — Concurrency & Cryptography

C projects from the Operating Systems course (2nd year, Braude College of Engineering).

## Projects

### [`proccsses and threads/`](./proccsses%20and%20threads/)

A hierarchical multi-level shell (`LibShell`) with three sub-shells, demonstrating POSIX process management:

- `fork()` + `execl()` / `execlp()` for process creation and program execution
- `wait()` for synchronization
- Direct system calls: `unlink()`, `chmod()`, `open()` / `read()` / `write()`

### [`crypto and auth/`](./crypto%20and%20auth/)

A file encryptor/decryptor with password authentication:

- **ChaCha20** stream cipher (RFC 8439) for encryption/decryption
- **SHA-256** (FIPS 180-4) for passcode verification — digest-only storage, constant-time comparison
- Random nonce per encryption (`/dev/urandom`), stored as file header

## Build

Each project compiles with `gcc` and has no external dependencies — only POSIX/libc.

```bash
# Processes & threads
cd "proccsses and threads"
./run_lib.sh

# Crypto & auth
cd "crypto and auth"
make
```
