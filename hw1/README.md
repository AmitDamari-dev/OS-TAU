# Homework 1: `hw1shell` - Build, Test, and Submission Manual

Course: **Operating Systems (0512.4402), Tel Aviv University**

---

## 1. Overview
`hw1shell` is a custom Unix shell written in C supporting internal commands (`cd`, `jobs`, `exit`), external command execution via `fork()` and `execvp()`, background asynchronous tasks (`&`), and automatic zombie reaping with a 4-process background capacity limit.

---

## 2. Directory Structure
```
hw1/
├── README.md                 # This manual
├── src/                      # Source files (maps directly to Moodle submission folder)
│   ├── Makefile              # Warning-free Makefile (-Wall -Wextra -Werror -pedantic)
│   ├── hw1shell.c            # Complete shell implementation
│   └── hw1shell.h            # Header definitions and function prototypes
├── tests/
│   └── run_tests.sh          # Comprehensive automated test suite + Valgrind checks
└── scripts/
    └── package_submission.sh # Submission packaging & validation script
```

---

## 3. Building and Running

### Build
```bash
cd hw1/src
make
```

### Clean
```bash
make clean
```

### Run
```bash
./hw1shell
```

---

## 4. Running the Automated Test Suite
From the repository root or `hw1/tests/`:
```bash
cd hw1/tests
bash run_tests.sh
```
The test suite validates:
1. Prompt display & empty line handling (Guideline 1, 10).
2. Internal `cd` command (0 args, 2 args, invalid dir, valid dir, `cd ..`) (Guideline 3, 13).
3. External foreground commands (`echo`, `ls`, etc.) (Guideline 7, 8).
4. Invalid external command error reporting & child termination (Guideline 11, 13).
5. Background job execution (`sleep 1 &`, `sleep 1&`), `jobs` listing, and completion reaping (Guidelines 4, 5, 9, 12).
6. 4-job background capacity enforcement (Guideline 6).
7. Exit waiting for active background jobs (Guideline 2).
8. Memory leak and file descriptor checks via Valgrind.

---

## 5. Submission Packaging
```bash
cd hw1/scripts
bash package_submission.sh <ID1> [ID2]
```
Generates and verifies `hw1_<ID1>[_<ID2>].tgz` in compliance with TAU Moodle grading requirements.
