# LibShell — Multi-Level Shell with Processes & Threads

A hierarchical shell system demonstrating OS process management: `fork()`, `exec*()`, and `wait()` in C.

## Architecture

```
libshell          ← top-level shell (parent process)
├── reader_shell  ← sub-shell: system info commands
├── catalog_shell ← sub-shell: file search & directory commands
└── archive_shell ← sub-shell: file archive/management commands
```

Each sub-shell runs as a **child process** spawned by `fork()` + `execl()`. The parent waits with `wait()` until the child exits (triggered by `Esc`).

## Build & Run

```bash
chmod +x run_lib.sh
./run_lib.sh
```

Or compile manually:

```bash
gcc -o libshell    libshell.c
gcc -o reader_shell  reader_shell.c
gcc -o catalog_shell catalog_shell.c
gcc -o archive_shell archive_shell.c
./libshell
```

## Commands

**LibShell** (`Lib$**`)

| Command   | Effect                          |
|-----------|---------------------------------|
| `reader`  | Enter Reader sub-shell          |
| `catalog` | Enter Catalog sub-shell         |
| `archive` | Enter Archive sub-shell         |
| `exit`    | Exit LibShell                   |

**Reader Shell** (`Reader$**`) — system info

| Command  | Effect                    |
|----------|---------------------------|
| `date`   | Print current date/time   |
| `whoami` | Print current user        |
| `pwd`    | Print working directory   |
| `uptime` | Print system uptime       |
| `Esc`    | Return to LibShell        |

**Catalog Shell** (`Catalog$**`) — search & directory

| Command              | Effect                              |
|----------------------|-------------------------------------|
| `find <word> <file>` | Search for word in file (`grep`)    |
| `ls -l`              | List directory contents (long fmt)  |
| `newcat <name>`      | Create new directory (`mkdir`)      |
| `run <prog> <arg>`   | Execute arbitrary program           |
| `Esc`                | Return to LibShell                  |

**Archive Shell** (`Archive$**`) — file management

| Command                | Effect                              |
|------------------------|-------------------------------------|
| `merge <src> <dst>`    | Append src contents to dst          |
| `count <file>`         | Count lines/words/chars (`wc`)      |
| `remove <file>`        | Delete file (`unlink`)              |
| `protect <mode> <file>`| Change permissions (`chmod`, octal) |
| `Esc`                  | Return to LibShell                  |

## OS Concepts Demonstrated

- **`fork()`** — process creation; each command runs in an isolated child
- **`execl()` / `execlp()`** — replace child image with target program
- **`wait()`** — parent blocks until child terminates (no zombie processes)
- **`unlink()` / `chmod()`** — direct POSIX system calls for file operations
- **File descriptors** — `open()`/`read()`/`write()`/`close()` for the `merge` command
