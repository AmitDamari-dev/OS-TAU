# OS-TAU: Operating Systems (0512.4402) - Tel Aviv University

Repository for Tel Aviv University's **Operating Systems (0512.4402)** coursework, programming labs, and XV6 kernel experiments.

---

## 📌 Course Dashboard & Assignments

| Assignment | Topic | Status | Language / Env | Directory |
| :--- | :--- | :---: | :---: | :--- |
| **HW1** | Custom Linux Shell (`hw1shell`), Process Lifecycle & Job Control | 🟢 In Progress | C99 / POSIX Linux | [`hw1/`](./hw1/) |
| **HW2** | Multithreading & Synchronization (`pthreads`) | ⚪ Planned | C99 / POSIX Threads | `hw2/` |
| **HW3** | XV6 System Calls & Process Management | ⚪ Planned | C / XV6 (x86) | `hw3/` |
| **HW4** | XV6 Memory Management & Paging | ⚪ Planned | C / XV6 (x86) | `hw4/` |
| **HW5** | File Systems & Storage | ⚪ Planned | C / XV6 / Linux | `hw5/` |

---

## 🛠️ Environment & Prerequisites

The coursework runs in a standard Linux POSIX environment (Ubuntu 22.04 LTS recommended) and later in the MIT XV6 x86 teaching operating system via QEMU.

### Essential Packages
```bash
sudo apt update
sudo apt install -y build-essential manpages-dev qemu-system git net-tools valgrind gdb
```

### Compiler Standards
- Compiler: `gcc`
- Flags: `-Wall -Wextra -Werror -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L -g`
- Strict Policy: Treat compiler warnings as fatal errors. Zero compiler warnings allowed.
- Zero Leaks: Every dynamic memory allocation must be freed, and all file descriptors and child processes reaped.

---

## 📂 Repository Structure

```
OS-TAU/
├── README.md                     # Semester dashboard, assignment status, environment quickstart
├── AGENTS.md                     # Persistent AI Agent instructions, C coding standards, TAU rules
├── .gitignore                    # Ignore *.o, .tgz, executables, valgrind logs, core dumps
├── docs/
│   ├── COURSE_STANDARDS.md       # Syscall error handling patterns, Makefile rules, tar packaging specs
│   └── hw1/
│       ├── hw1.pdf               # Official HW1 assignment specification PDF
│       └── DESIGN.md             # Deep-dive architecture, state machine & edge-case analysis
└── hw1/
    ├── README.md                 # HW1 build, test, and submission manual
    ├── src/                      # Clean source directory (packaged into hw1_<id1>[_<id2>])
    │   ├── Makefile
    │   ├── hw1shell.c
    │   └── hw1shell.h
    ├── tests/
    │   └── run_tests.sh          # Automated black-box, edge-case & Valgrind test suite
    └── scripts/
        └── package_submission.sh # Builds & verifies Moodle-compliant hw1_<id1>[_<id2>].tgz
```

---

## 🚀 Quickstart: Homework 1 (`hw1shell`)

### Build
```bash
cd hw1/src
make
```

### Run
```bash
./hw1shell
```

### Run Automated Tests & Valgrind
```bash
cd hw1/tests
bash run_tests.sh
```

### Package Submission for Moodle
```bash
cd hw1/scripts
bash package_submission.sh <ID1> [ID2]
```
