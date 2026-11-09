# Homework 1: `hw1shell` Architectural Design & Specification Analysis

Course: **Operating Systems (0512.4402), Tel Aviv University**  
Component: **hw1shell** (Custom Linux POSIX Shell)  
Author: **Staff Systems & Linux Kernel Engineer**

---

## 1. Executive Summary & Core Objectives

The goal of `hw1shell` is to implement a robust, POSIX-compliant user-space shell in C that manages process lifecycles, executes internal commands, runs external foreground/background programs via `fork()` and `execvp()`, handles concurrency up to 4 simultaneous background tasks, and eliminates zombie processes and resource leaks.

Automated evaluation scripts at TAU employ strict `diff` matching against standard output and inspect process tables to ensure that:
1. No zombie processes remain un-reaped.
2. Background slots are freed immediately upon task completion.
3. System call errors display exact names and preserved `errno` codes.
4. Output formatting matches the specification down to spaces and tabs.

---

## 2. High-Level Process Architecture & State Machine

```
                        +----------------------------+
                        |        hw1shell$           |
                        | (Print Prompt & Flush Out) |
                        +--------------+-------------+
                                       |
                                       v
                        +----------------------------+
                        |   Read Line from stdin     |
                        |      (fgets / EOF check)   |
                        +--------------+-------------+
                                       |
                  +--------------------+--------------------+
                  |                                         |
            (fgets == NULL)                         (Line read)
                  |                                         |
                  v                                         v
       +-----------------------+              +---------------------------+
       |   Graceful Exit:      |              | Preserve raw command line |
       | Reap all bg processes |              | Strip trailing \r and \n  |
       |     exit(0)           |              | Tokenize via " \t"        |
       +-----------------------+              +-------------+-------------+
                                                            |
                  +--------------------+--------------------+
                  |                    |                    |
             (0 tokens)        (Internal Command)    (External Command)
                  |                    |                    |
                  |         +----------+----------+         |
                  |         | exit |  cd  |  jobs |         |
                  |         +------+------+-------+         |
                  |                    |                    |
                  +--------------------+                    v
                                       |             Check Background (&)
                                       |             4-job capacity check
                                       |             fflush(stdout/stderr)
                                       |                    |
                                       |                    v
                                       |                 fork()
                                       |             +------+------+
                                       |             |             |
                                       |       (Child: 0)    (Parent: >0)
                                       |             |             |
                                       |       execvp(...)         |
                                       |       On error:           |
                                       |       print errs          |
                                       |       _exit(1)            |
                                       |                           v
                                       |                    Foreground / Bg:
                                       |                    FG: waitpid(pid)
                                       |                    BG: add to table
                                       |                        "started"
                                       |                           |
                                       +-------------+-------------+
                                                     |
                                                     v
                                      +-----------------------------+
                                      | End-of-Iteration Reaping    |
                                      | Poll bg jobs (WNOHANG)      |
                                      | Print "pid %d finished"     |
                                      | Free completed table slots  |
                                      +--------------+--------------+
                                                     |
                                                     +-----> (Next Loop Iteration)
```

---

## 3. Deep-Dive Design Decisions

### 3.1 Why `cd` Must Be an Internal Command (Guideline 3)
A common interview and exam question in Operating Systems:
> *Why does `cd` have to be an internal shell command rather than an external binary?*

**Operating System Mechanics:**
- Every process in Unix/Linux maintains its own current working directory (CWD) inside its kernel `struct task_struct` (specifically in `task->fs->pwd`).
- When a process calls `fork()`, the kernel duplicates the calling process. The child inherits a copy of the parent's working directory pointer.
- If `cd` were an external executable (e.g. `/bin/cd`), the shell would fork a child process, and that child would execute `chdir()`. The child's CWD would change, and then the child would exit.
- When control returns to the parent shell process, the shell's own working directory remains completely unchanged.
- Therefore, changing the working directory of the shell environment MUST be executed within the context of the shell process itself using the `chdir()` system call.

### 3.2 Standard Output vs. Standard Error Policy
TAU grading scripts use automated test harnesses that capture output for comparison.
- **Decision:** All operational outputs, prompt strings, and error messages (`hw1shell: ...`) are printed directly to `stdout` followed immediately by `fflush(stdout)`.
- **Pre-fork Safety:** Before executing `fork()`, both `stdout` and `stderr` are explicitly flushed via `fflush(stdout); fflush(stderr);`. When standard output is redirected to a pipeline or grading file, the C standard library switches from line-buffered to block-buffered mode. Without explicit flushing, unwritten buffer data is duplicated into the child's address space upon `fork()`, resulting in duplicated prompts or error lines.

### 3.3 Raw Command Preservation for `jobs` (Guideline 4 & 5)
Guideline 4 states:
> *"Each line should contain the pid, then a tab, then the background command entered by the user."*

When a user enters a background command such as `sleep 10 &` or `sleep 10&`:
1. The shell reads the raw line into `char line[1025]`.
2. Before `strtok()` mutates `line` with null terminators, the trimmed line is preserved verbatim in `char raw_cmd[1025]`.
3. When parsing arguments:
   - If the last token is `"&"`, `argv[last_idx]` is replaced with `NULL`.
   - If the last token ends with `'&'` (e.g. `sleep 10&`), the trailing `'&'` is replaced with `'\0'`.
4. In the jobs table, storing the preserved user input guarantees that `jobs` prints `%d\t%s\n` with the exact command entered by the user as specified.

### 3.4 Process Lifecycle & Zero-Leak Guarantees

#### Child Process Termination via `_exit()`
If `execvp()` fails:
- Guideline 11: Print `hw1shell: invalid command\n`
- Guideline 13: Print `hw1shell: execvp failed, errno is %d\n`
- Flush stdout/stderr (`fflush(NULL)`).
- Call `_exit(1)` immediately. Calling standard `exit()` or returning from main would trigger C runtime cleanup handlers and cause duplicate execution loops in the child.

#### Background Reaping Table (`WNOHANG`)
- Fixed table of 4 slots: `bg_job_t bg_jobs[4]`.
- Each slot tracks `pid_t pid` and `char cmd[1025]`.
- At the end of every loop iteration (even after empty commands, failed commands, or internal commands), the shell polls each active slot with:
  ```c
  pid_t ret = waitpid(bg_jobs[i].pid, &status, WNOHANG);
  ```
- If `ret > 0`: The background child finished and was reaped. The shell prints `hw1shell: pid %d finished\n` and zeroes out the slot, freeing capacity for subsequent commands.
- If `ret == -1`: If `errno == ECHILD`, the child is already gone; print syscall error and clear slot.

#### Clean Termination on `exit` and `EOF`
Guideline 2 requires that upon `exit` (or EOF / Ctrl+D):
- The shell waits for and reaps ALL currently active background processes using blocking `waitpid(bg_jobs[i].pid, &status, 0)`.
- If `waitpid` fails, report `hw1shell: waitpid failed, errno is %d\n`.
- Any heap-allocated memory is released.
- Shell terminates with `exit(0)`.

---

## 4. Complete Error Message & Syscall Catalog

| Trigger Event | Guideline | Exact Output Format | Syscall Error (Guideline 13) |
| :--- | :---: | :--- | :--- |
| **Shell Prompt** | 1 | `hw1shell$ ` *(no newline, flushed)* | None |
| **Invalid `cd` Syntax** | 3 | `hw1shell: invalid command\n` | None |
| **`chdir` Failure** | 3, 13 | `hw1shell: invalid command\n` | `hw1shell: chdir failed, errno is %d\n` |
| **`jobs` Display** | 4 | `%d\t%s\n` *(PID, tab, cmd string)* | None |
| **5th Background Job** | 6 | `hw1shell: too many background commands running\n` | None (no fork attempted) |
| **`fork` Failure** | 7, 13 | None | `hw1shell: fork failed, errno is %d\n` |
| **Foreground `waitpid` Fail**| 8, 13 | None | `hw1shell: waitpid failed, errno is %d\n` |
| **Background Job Started** | 9 | `hw1shell: pid %d started\n` | None |
| **Empty Input Line** | 10 | None *(redisplay prompt after reaping)* | None |
| **Unknown External Command**| 11, 13| `hw1shell: invalid command\n` | `hw1shell: execvp failed, errno is %d\n` |
| **Background Job Finished** | 12 | `hw1shell: pid %d finished\n` | None |
| **Background `waitpid` Fail**| 12, 13| None | `hw1shell: waitpid failed, errno is %d\n` |

---

## 5. Edge-Case Analysis Matrix

1. **Whitespace Only / Bare Enter:**
   - Input: `"   \t   \n"` or `"\n"`
   - Result: 0 tokens. No error printed. Reaps any background jobs that finished, redisplays prompt.
2. **`cd` Edge Cases:**
   - `cd` (0 args): `hw1shell: invalid command\n`
   - `cd a b` (2 args): `hw1shell: invalid command\n`
   - `cd non_existent_dir_xyz`: `hw1shell: invalid command\n` followed by `hw1shell: chdir failed, errno is 2\n`
   - `cd ..`: Changes to parent directory; verified by `/bin/pwd`.
3. **Background Syntax Variations:**
   - `sleep 5 &`: Token 0 is `sleep`, token 1 is `5`, token 2 is `&`. `&` stripped from `argv`.
   - `sleep 5&`: Token 0 is `sleep`, token 1 is `5&`. Token 1 modified in-place to `5`.
4. **Max Background Limit:**
   - 4 concurrent `sleep 10 &` commands fill all 4 slots.
   - 5th background command receives `hw1shell: too many background commands running\n` without forking.
   - Pressing Enter after sleep commands finish reaps all completed slots and prints `hw1shell: pid %d finished\n`.
5. **EOF / Ctrl+D:**
   - `fgets()` returns `NULL`. Shell gracefully reaps background jobs and exits with code 0.
