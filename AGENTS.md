# Persistent AI Agent Guidelines: OS-TAU (0512.4402)

This document governs the engineering standards, architecture decisions, and workflow guidelines for all AI agents (Codex, Claude, Antigravity, Gemini) collaborating in the `OS-TAU` repository.

---

## 1. Academic & Course Context
- **Course:** Tel Aviv University - 0512.4402 Operating Systems.
- **Target OS:** Linux (Ubuntu 22.04 LTS / POSIX 200809L) and XV6 (x86 QEMU emulator).
- **Core Principle:** Exact compliance with course specifications. Grading uses automated `diff` checkers and process inspection scripts. Any variation in stdout/stderr messages or exit codes causes test failure.

---

## 2. C Systems Engineering Standards

### 2.1 Compilation & Compiler Flags
Every C source file must compile cleanly under:
```bash
gcc -Wall -Wextra -Werror -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L -g
```
- **Treat Warnings as Errors:** Any code producing a compiler warning is strictly rejected.
- **Standards Compliance:** Write pure POSIX C99 code. Do not use GNU extensions unless explicitly permitted.
- **Debugging Symbols:** Always compile with `-g` to facilitate `gdb` and `valgrind` tracing.

### 2.2 System Call Error Handling (TAU Guideline 13)
Whenever a POSIX system call (`fork`, `execvp`, `waitpid`, `chdir`, etc.) returns an error (`-1` or `NULL`):
1. Immediately preserve `errno` into a local variable (`int saved_errno = errno;`) before any subsequent function call (e.g. `printf`) can overwrite it.
2. Output the exact required error string:
   ```c
   printf("hw1shell: %s failed, errno is %d\n", syscall_name, saved_errno);
   fflush(stdout);
   ```
3. If an assignment guideline also specifies an application-level error message (such as `hw1shell: invalid command\n`), output both messages in accordance with Guideline 13's additive rule.

### 2.3 Process Management & Resource Leaks
- **No Zombies:** Every process created with `fork()` must be reaped via `waitpid()`. Background processes must be polled periodically with `WNOHANG` and reaped cleanly.
- **Fork Buffer Safety:** Always call `fflush(stdout); fflush(stderr);` immediately prior to `fork()` to prevent duplicated buffer content when stdout is redirected to a pipe.
- **Child Termination:** If `execvp()` fails in a child process, report the error and terminate immediately using `_exit(1)` (never `exit()`, and never return to the parent shell loop).
- **Zero Memory Leaks:** Every allocation via `malloc`/`calloc`/`strdup` must have a guaranteed corresponding `free()`. Use `valgrind --leak-check=full --track-origins=yes` to verify.

---

## 3. Makefile Standards
- Commands must be indented with a literal **TAB** character (`\t`), never spaces.
- Target `all` must be the default target and build the primary executable.
- Target `clean` must use `rm -f *.o <executable>` and never fail if files do not exist.
- Use pattern rules (`%.o: %.c`) and declare non-file targets with `.PHONY: all clean`.

---

## 4. Moodle Submission Packaging Rules
- Root submission folder naming: `hw1_<id1>` (single) or `hw1_<id1>_<id2>` (pairs).
- Contents: ONLY `Makefile`, `hw1shell.c`, `hw1shell.h` (and any necessary `.c`/`.h` files). No object files, binaries, or nested subdirectories.
- Archive format: `.tgz` (`tar -zcvf hw1_<id1>[_<id2>].tgz hw1_<id1>[_<id2>]`).
- Verification: Always extract into a clean `/tmp` directory and run `make && make clean` before submission.

---

## 5. Git Commit Hygiene
Use Conventional Commits:
- `feat(...)`: New features or assignment implementations
- `fix(...)`: Bug fixes or edge-case corrections
- `test(...)`: Test suite additions and verification scripts
- `docs(...)`: Architectural design docs, specs, and README updates
- `chore(...)`: Repository configuration, toolchains, build scripts
