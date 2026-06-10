# Operating Systems — Concurrency & Cryptography

C projects from the Operating Systems course (2nd year, Braude College of Engineering).

## Projects

### [`concurrent-systems/`](./concurrent-systems/)

Lab assignments covering concurrency, synchronization, and multi-threading:

- **processes-threads** — Hierarchical shell demonstrating POSIX process/thread management
- **dining-philosophers** — Classic synchronization problem with mutex/semaphore solutions
- **semaphores** — Semaphore patterns for critical section protection
- **thread-safe-stack** — Stack implementation with thread-safe operations
- **matrix-threads** — Parallel matrix operations using threads
- **cpu-runtime** — Process scheduling and CPU time measurement
- **binary-tree** — Concurrent tree traversal and manipulation
- **file-encryptor** — File encryption with concurrent I/O
- **shell-scripting** — System-level shell scripting

### [`garage-simulator/`](./garage-simulator/)

Multi-threaded parking garage simulation with dynamic capacity management:

- Producer-consumer pattern with thread synchronization
- Queue management for vehicle entry/exit
- Real-time garage state tracking and reporting
- POSIX threads and mutex synchronization

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
# Concurrent systems labs
cd concurrent-systems/<project-name>
make

# Garage simulator
cd garage-simulator
make

# Processes & threads
cd "proccsses and threads"
./run_lib.sh

# Crypto & auth
cd "crypto and auth"
make
```
