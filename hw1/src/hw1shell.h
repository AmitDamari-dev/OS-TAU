/**
 * @file hw1shell.h
 * @brief Tel Aviv University - Operating Systems (0512.4402) - Homework 1
 *
 * Header definitions, data structures, constants, and function prototypes
 * for hw1shell, a POSIX-compliant user-space shell.
 *
 * Compliant with Homework 1 Guidelines 1 through 14.
 */

#ifndef HW1SHELL_H
#define HW1SHELL_H

#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>

/**
 * Maximum line length supported as per Guideline 14.
 * Buffer size is MAX_LINE_LEN + 1 to safely accommodate the null terminator.
 */
#define MAX_LINE_LEN 1024

/**
 * Maximum command-line arguments supported as per Guideline 14.
 * Argument array size is MAX_ARGS + 1 so index 64 can hold the NULL sentinel for execvp.
 */
#define MAX_ARGS 64

/**
 * Maximum simultaneous background jobs supported as per Guideline 6.
 */
#define MAX_BG_JOBS 4

/**
 * Interactive shell prompt string as per Guideline 1.
 */
#define SHELL_PROMPT "hw1shell$ "

/**
 * @struct bg_job
 * @brief Represents an active background job tracked in the jobs table.
 *
 * Satisfies Guidelines 4, 6, 9, and 12.
 */
typedef struct {
    pid_t pid;                       /**< Process ID of the background child (>0 if active, 0 if free) */
    char cmd[MAX_LINE_LEN + 1];     /**< Preserved raw command string entered by the user */
} bg_job_t;

/**
 * @brief Prints the shell prompt and flushes stdout immediately.
 * Satisfies Guideline 1.
 */
void print_prompt(void);

/**
 * @brief Safely logs a system call failure using the preserved errno value.
 * Satisfies Guideline 13.
 *
 * @param syscall_name Name of the failing system call (e.g., "chdir", "fork", "waitpid", "execvp").
 * @param err Preserved value of errno immediately after the syscall failed.
 */
void print_syscall_error(const char *syscall_name, int err);

/**
 * @brief Handles the internal 'cd' command.
 * Satisfies Guideline 3 and Guideline 13.
 *
 * Must be executed in the shell process itself because a child process inherits
 * its own copy of the working directory; changing directory in a child would not
 * alter the parent shell's working directory.
 *
 * @param argc Number of tokens in argv.
 * @param argv Tokenized argument array (argv[0] is "cd").
 * @return 0 on success, -1 on invalid command or chdir error.
 */
int handle_cd(int argc, char **argv);

/**
 * @brief Handles the internal 'jobs' command by listing all active background processes.
 * Satisfies Guideline 4.
 *
 * Prints lines formatted as: "%d\t%s\n" (PID, tab, command string).
 *
 * @param jobs Pointer to the background jobs array.
 * @param max_jobs Total capacity of the background jobs array (MAX_BG_JOBS).
 */
void handle_jobs(const bg_job_t *jobs, int max_jobs);

/**
 * @brief Handles the internal 'exit' command and EOF condition.
 * Satisfies Guideline 2.
 *
 * Blocks and reaps all active background child processes using waitpid(),
 * performs memory/resource cleanups, and terminates with exit(0).
 *
 * @param jobs Pointer to the background jobs array.
 * @param max_jobs Total capacity of the background jobs array.
 */
void handle_exit(bg_job_t *jobs, int max_jobs);

/**
 * @brief Reaps any background child processes that have completed without blocking.
 * Satisfies Guideline 12 and Guideline 13.
 *
 * Uses waitpid(pid, &status, WNOHANG) at the end of every main loop iteration.
 * Displays "hw1shell: pid %d finished\n" for each reaped child and clears its slot.
 *
 * @param jobs Pointer to the background jobs array.
 * @param max_jobs Total capacity of the background jobs array.
 */
void reap_finished_bg_jobs(bg_job_t *jobs, int max_jobs);

/**
 * @brief Finds the first available slot in the background jobs table.
 * Satisfies Guideline 6.
 *
 * @param jobs Pointer to the background jobs array.
 * @param max_jobs Total capacity of the background jobs array.
 * @return Available slot index (0 to max_jobs - 1), or -1 if the table is full.
 */
int find_free_bg_slot(const bg_job_t *jobs, int max_jobs);

#endif /* HW1SHELL_H */
