/*
 * hw1shell.h - constants, background-job table type and prototypes.
 */

#ifndef HW1SHELL_H
#define HW1SHELL_H

#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>

#define MAX_LINE_LEN 1024        /* max input line length (guideline 14) */
#define MAX_ARGS 64              /* max parameters per command (guideline 14) */
#define MAX_BG_JOBS 4            /* max concurrent background jobs (guideline 6) */
#define SHELL_PROMPT "hw1shell$ "

/* One slot in the background jobs table. pid == 0 means the slot is free. */
typedef struct {
    pid_t pid;
    char cmd[MAX_LINE_LEN + 1];  /* command exactly as the user typed it */
} bg_job_t;

/* Print the prompt and flush stdout. */
void print_prompt(void);

/* Print "hw1shell: <name> failed, errno is <err>" (guideline 13). */
void print_syscall_error(const char *syscall_name, int err);

/* Internal cd. Returns 0 on success, -1 on a bad command or chdir failure. */
int handle_cd(int argc, char **argv);

/* Internal jobs: print "<pid>\t<command>" for each running background job. */
void handle_jobs(const bg_job_t *jobs, int max_jobs);

/* Internal exit: wait for all background jobs, then exit(0). */
void handle_exit(bg_job_t *jobs, int max_jobs);

/* Reap finished background jobs without blocking and free their slots. */
void reap_finished_bg_jobs(bg_job_t *jobs, int max_jobs);

/* Index of the first free slot, or -1 if the table is full. */
int find_free_bg_slot(const bg_job_t *jobs, int max_jobs);

#endif /* HW1SHELL_H */
