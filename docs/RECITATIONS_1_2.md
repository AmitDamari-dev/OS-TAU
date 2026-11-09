# Course Recitations 1 & 2: Linux, Shell, Makefiles, Submissions & XV6

Course: **0512.4402 Operating Systems - Tel Aviv University**  
Target Environments: **Linux (Ubuntu 22.04 LTS) & XV6 (MIT x86 Teaching OS / QEMU)**

---

## Section 1: System Environment & Virtualization (Recitation 1)

### 1.1 Virtualization Prerequisites & BIOS Configuration
- **Hardware Virtualization Support:** Running virtual machines (VirtualBox/VMware) or subsystem tools (WSL2) requires hardware virtualization extensions (Intel VT-x or AMD-V) enabled in BIOS/UEFI.
- **Verification:** Windows Task Manager $\rightarrow$ Performance $\rightarrow$ CPU $\rightarrow$ `Virtualization: Enabled`.

### 1.2 Environment Options Breakdown
- **Option 1: Oracle VirtualBox + Ubuntu 22.04 LTS (Course Standard)**
  - Why 22.04 LTS: Newer Linux releases ship with GCC >= 14 which breaks the legacy compilation configuration of the course's XV6 build.
  - University Labs: Pre-configured image located at `C:\Users\Public\Documents\ubuntu_22.04_vb.vdi` (Tochna 1, 8, 9, 10, Kitot 3).
  - Credentials (OSBoxes): Username `osboxes`, Password `osboxes.org`.
  - Shared Folders: Named `vmshared`. Inside guest run `sudo adduser $USER vboxsf`.
- **Option 2: Windows Subsystem for Linux (WSL2)**
  - Direct file interoperability with Windows via `\\wsl$\Ubuntu`.
  - Running `explorer.exe .` in WSL opens Windows Explorer in the current Linux directory.
- **Option 3: Native Linux Dual-Boot Installation**

### 1.3 Essential Toolchain Packages
```bash
sudo apt update
sudo apt upgrade
sudo apt install -y build-essential manpages-dev qemu-system git net-tools wget ca-certificates valgrind gdb
```

---

## Section 2: Linux Terminal, Bash & File Management (Recitation 2)

### 2.1 The Shell Architecture
The shell (`bash`) is a standard user program that reads user input, executes the requested binary, and waits for completion.
- **Built-in vs. External Commands:** Some commands are handled directly by the shell itself (`cd`, `history`), whereas others exist as independent binary files on disk (`ls`, `ps`). Check via `type cd` vs `type ls`.
- **Manual Pages (`man`):**
  - Section 1: User commands (`man ls`)
  - Section 2: Kernel system calls (`man 2 fork`)
  - Section 3: C library functions (`man 3 printf`)

### 2.2 Directory Navigation & File Utilities
- `pwd`: Print current working directory absolute path.
- `mkdir -p os/rec2`: Create directory along with any missing parent directories.
- `cd ..`: Move to parent directory.
- `cd ~` (or `cd`): Move to user home directory.
- `cd -`: Switch back to the previously active directory.
- `ls -la`: Long listing format showing hidden files (`.` and `..`).
- `cp`, `mv`, `rm`: Note that Unix deletion (`rm`) is permanent.
- `cat`, `less`, `head -n 5`, `tail -n 5`.
- Globbing: `ls *.c`, grep: `grep -n main *.c`.

### 2.3 File Permissions & Ownership
Format `-rwxr-xr-x`: File type, owner (rwx), group (r-x), others (r-x).
- `chmod +x script.sh`: Add execute permissions.

### 2.4 Keyboard Shortcuts & Productivity
- `Tab`: Auto-complete file and command names.
- `Ctrl-R`: Reverse-incremental search through previous commands.
- `Ctrl-C`: Immediately abort/terminate running foreground program (`SIGINT`).
- `Ctrl-D`: Signal End-Of-File (`EOF`) to interactive standard input.
- `Ctrl-Z`: Suspend running foreground program (`SIGTSTP`).
- `Ctrl-L`: Clear screen buffer.

### 2.5 Job Control (`&`, `jobs`, `fg`, `bg`, `kill`)
- Appending `&` runs command asynchronously in the background (`sleep 100 &`).
- `jobs`: Lists active background job IDs and status.
- `fg`: Brings background job to foreground.
- `bg`: Resumes suspended job in background.
- `kill %1` or `kill <PID>`: Terminate job.
- *HW1 Core Objective:* Implementing a custom shell capable of managing background jobs (`&`), listing active processes (`jobs`), and printing start/completion messages.

### 2.6 Redirection, Pipes, and Exit Codes
- `>` (overwrite stdout), `>>` (append stdout), `<` (stdin redirection), `2>` (stderr).
- `|` (Pipe): Connects stdout of one process to stdin of next process.
- `echo $?`: Prints exit status code of preceding command (0 = success, non-zero = error).

### 2.7 The `$PATH` Environment Variable
- `echo $PATH`: Colon-separated directories where the shell searches for binaries.
- To execute a local binary in the current directory: `./prog`.

---

## Section 3: Compiling C Programs with GCC

### 3.1 Course Compilation Standards
```bash
gcc -Wall -Wextra -Werror -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L -g source.c -o prog
```
- `-Wall -Wextra`: Enable all major compiler warning diagnostics.
- `-Werror`: Treat warnings as fatal errors. Code compiling with warnings loses points.
- `-g`: Embed debug symbols for `gdb` and `valgrind`.
- `-pthread`: Required flag for POSIX threads at compile and link stages (HW2).

### 3.2 Compilation vs Linking
- **Compilation (`-c`):** Translates `.c` into `.o` machine code. Catches syntax and declaration errors.
- **Linking:** Combines `.o` files and libraries into an executable. Catches `undefined reference` errors.

---

## Section 4: Automating Builds with Make and Makefiles

- **Syntax Rule:** Command recipes must be indented with a real ASCII **TAB** (`\t`). Spaces cause `*** missing separator. Stop.`
- **Automatic Variables:** `$@` (target name), `$<` (first prerequisite), `$^` (all prerequisites).
- **Pattern Rules:** `%.o: %.c` builds object files from C source.
- **Phony Targets:** `.PHONY: all clean` prevents conflicts with files named `all` or `clean`.
- **Clean Target:** `rm -f *.o prog` guarantees idempotent cleanup.

---

## Section 5: Homework Submission Guidelines (Moodle)

- Directory naming: `hw1_<id1>` or `hw1_<id1>_<id2>`.
- Contents: ONLY `Makefile`, `.c`, `.h` source files. No binaries or object files.
- Archive: `tar -zcvf hw1_<id1>_<id2>.tgz hw1_<id1>_<id2>`
- Verification: Extract into `/tmp`, run `make && make clean`.

---

## Section 6: XV6 & QEMU Emulator (Upcoming Modules)

- Clone & Build:
  ```bash
  git clone https://github.com/mit-pdos/xv6-public.git
  cd xv6-public
  make
  ```
- Run: `make qemu` (graphical) or `make qemu-nox` (text/console mode).
- Exit QEMU text mode: Press `Ctrl-a`, then `x`.
- Custom user programs: Added under `UPROGS` in the XV6 Makefile.
