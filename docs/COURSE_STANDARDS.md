# Tel Aviv University Operating Systems (0512.4402) - Course Engineering Standards

This document formalizes the software architecture, compilation rules, POSIX standards, and submission protocols across all assignments in course 0512.4402.

---

## 1. System Call Error Handling Conventions

Grading scripts evaluate output with strict `diff` checks and monitor process tables. System call errors must be handled consistently across all assignments:

### 1.1 The Golden Rule of `errno`
In C and POSIX systems, `errno` is a thread-local global variable modified by failing system calls and C library functions. Any subsequent library call (including `printf()`, `malloc()`, or `close()`) can overwrite `errno`.

**Pattern:**
```c
int res = some_syscall(...);
if (res == -1) {
    int saved_errno = errno; /* Save immediately */
    /* Log or handle using saved_errno */
    printf("hw1shell: some_syscall failed, errno is %d\n", saved_errno);
    fflush(stdout);
}
```

### 1.2 Guideline 13 Additive Rule
In assignments with explicit application-level errors (e.g. `hw1shell: invalid command\n`), if a system call failure caused that error (e.g. `chdir` or `execvp`), the course specification states:
> *"This error message should be displayed in addition to other error messages displayed in accordance with previous guidelines."*

Therefore, both messages must be printed in deterministic order:
1. Application error: `hw1shell: invalid command\n`
2. Syscall error: `hw1shell: <syscall_name> failed, errno is %d\n`

### 1.3 Pre-Fork Output Buffer Flushing
When stdout is redirected to a file or pipe (as in automated grading harnesses), standard I/O streams switch from line-buffered to fully-buffered. If `fork()` is executed without flushing, unflushed buffer contents are duplicated into the child address space, resulting in duplicated output lines.
**Mandatory pre-fork safety:**
```c
fflush(stdout);
fflush(stderr);
pid_t pid = fork();
```

### 1.4 Child Failure Termination via `_exit()`
If `execvp()` fails in a child process, the child must NEVER call `exit()` or return to the main shell loop. Calling `exit()` can invoke `atexit` handlers or flush parent buffers improperly; returning would spawn a duplicate shell process reading from the same stdin. Always terminate the child using:
```c
_exit(1);
```

---

## 2. Makefile Architecture & Syntax

Every assignment submission requires a portable, warning-free Makefile.

### 2.1 Compiler Flags
- `-Wall`: Enables all major warnings.
- `-Wextra`: Enables extra warning diagnostics.
- `-Werror`: Treats warnings as hard errors (enforced in CI/testing).
- `-pedantic`: Demands strict ISO C compliance.
- `-std=c99`: Targets ISO/IEC 9899:1999 standard.
- `-D_POSIX_C_SOURCE=200809L`: Exposes POSIX.1-2008 APIs without non-standard pollution.
- `-g`: Generates comprehensive DWARF debug symbols.

### 2.2 Rules & Formatting
- All indentation in command recipes MUST use raw ASCII Tab characters (`\t`).
- Targets `all` and `clean` must be declared `.PHONY`.
- Target `clean` must use `rm -f *.o <executable>` to guarantee a zero exit code even when cleaning a pristine directory.

---

## 3. Moodle Tarball Packaging Specifications

TAU automated grading systems extract submissions into sandboxed grading environments:
1. The tarball filename must match `hw1_<id1>.tgz` or `hw1_<id1>_<id2>.tgz`.
2. The archive must contain a single top-level directory named identically to the archive base (`hw1_<id1>[_<id2>]`).
3. Inside this directory, only the Makefile and source code (`.c`, `.h`) files should exist.
4. Packaging command:
   ```bash
   tar -zcvf hw1_<id1>_<id2>.tgz hw1_<id1>_<id2>
   ```
5. Verification pipeline:
   ```bash
   mkdir -p /tmp/verify && tar -zxvf hw1_<id1>_<id2>.tgz -C /tmp/verify
   cd /tmp/verify/hw1_<id1>_<id2>
   make && make clean
   ```

---

## 4. XV6 / QEMU Environment Standards (Recitations 1 & 2)

Later course modules operate on the MIT XV6 x86 instructional operating system:
- **Build:** `make` inside `xv6-public`.
- **Run (Console / Text Mode):** `make qemu-nox` (optimal for WSL2 and headless servers).
- **Exit QEMU:** Press `Ctrl-a`, then `x`.
- **Toolchain:** GCC on Ubuntu 22.04 LTS (avoid GCC >= 14 for legacy XV6 builds).
