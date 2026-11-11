/**
 * @file hw1shell.c
 * Simple shell: internal commands cd/jobs/exit, external commands via
 * fork/execvp, and up to 4 concurrent background jobs.
 */

#include "hw1shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

/**
 * @brief Prints the shell prompt and flushes stdout immediately.
 * Guideline 1: The shell runs an infinite loop that writes "hw1shell$ " to stdout.
 */
void print_prompt(void) {
    printf(SHELL_PROMPT);
    fflush(stdout);
}

/**
 * @brief Prints system call error messages with preserved errno.
 * Guideline 13: "hw1shell: %s failed, errno is %d\n"
 *
 * @param syscall_name The exact name of the failed system call.
 * @param err The preserved value of errno.
 */
void print_syscall_error(const char *syscall_name, int err) {
    printf("hw1shell: %s failed, errno is %d\n", syscall_name, err);
    fflush(stdout);
}

/**
 * @brief Handles the internal 'cd' command.
 * Guideline 3: Local chdir implementation.
 *
 * ARCHITECTURAL DESIGN NOTE (Assignment Prompt):
 * "think why cd has to be an internal and not external command"
 *
 * cd must run inside the shell process itself: a child's chdir would only
 * change the child's own working directory, not the shell's.
 * @param argc Token count of the command.
 * @param argv Argument vector (argv[0] == "cd").
 * @return 0 on success, -1 on syntax or chdir error.
 */
int handle_cd(int argc, char **argv) {
    /* Guideline 3: cd requires exact syntax 'cd <directoryname>'. Any other variant is invalid. */
    if (argc != 2) {
        printf("hw1shell: invalid command\n");
        fflush(stdout);
        return -1;
    }

    if (chdir(argv[1]) == -1) {
        int saved_errno = errno;
        /* Guideline 3 requires invalid command message */
        printf("hw1shell: invalid command\n");
        /* Guideline 13 requires syscall error message in addition */
        print_syscall_error("chdir", saved_errno);
        return -1;
    }

    return 0;
}

/**
 * @brief Handles the internal 'jobs' command.
 * Guideline 4: Displays active background jobs as "%d\t%s\n".
 *
 * @param jobs Pointer to background jobs table.
 * @param max_jobs Capacity of background jobs table.
 */
void handle_jobs(const bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid > 0) {
            printf("%d\t%s\n", (int)jobs[i].pid, jobs[i].cmd);
        }
    }
    fflush(stdout);
}

/**
 * @brief Handles the internal 'exit' command and EOF termination.
 * Guideline 2: Wait and reap all background children before exiting, then exit(0).
 *
 * @param jobs Pointer to background jobs table.
 * @param max_jobs Capacity of background jobs table.
 */
void handle_exit(bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid > 0) {
            int status;
            pid_t res = waitpid(jobs[i].pid, &status, 0);
            if (res == -1) {
                int saved_errno = errno;
                print_syscall_error("waitpid", saved_errno);
            }
            jobs[i].pid = 0;
            jobs[i].cmd[0] = '\0';
        }
    }
    exit(0);
}

/**
 * @brief Reaps completed background jobs at the end of each iteration.
 * Guideline 12: Non-blocking check for terminated background children.
 * Prints "hw1shell: pid %d finished\n" and frees the slot.
 *
 * @param jobs Pointer to background jobs table.
 * @param max_jobs Capacity of background jobs table.
 */
void reap_finished_bg_jobs(bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid > 0) {
            int status;
            pid_t res = waitpid(jobs[i].pid, &status, WNOHANG);
            if (res > 0) {
                printf("hw1shell: pid %d finished\n", (int)jobs[i].pid);
                fflush(stdout);
                jobs[i].pid = 0;
                jobs[i].cmd[0] = '\0';
            } else if (res == -1) {
                int saved_errno = errno;
                print_syscall_error("waitpid", saved_errno);
                if (saved_errno == ECHILD) {
                    jobs[i].pid = 0;
                    jobs[i].cmd[0] = '\0';
                }
            }
        }
    }
}

/**
 * @brief Finds the first available background slot.
 * Guideline 6: Checks availability across MAX_BG_JOBS slots.
 *
 * @param jobs Pointer to background jobs table.
 * @param max_jobs Capacity of background jobs table.
 * @return Index of free slot (0..max_jobs-1), or -1 if full.
 */
int find_free_bg_slot(const bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Main entry point and lifecycle loop for hw1shell.
 */
int main(void) {
    bg_job_t bg_jobs[MAX_BG_JOBS];
    memset(bg_jobs, 0, sizeof(bg_jobs));

    char line[MAX_LINE_LEN + 1];
    char raw_cmd[MAX_LINE_LEN + 1];
    char *argv[MAX_ARGS + 1];

    while (1) {
        /* Guideline 1: Print prompt */
        print_prompt();

        /* Guideline 1 & 2: Accept user command from stdin; handle EOF (Ctrl+D) */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            handle_exit(bg_jobs, MAX_BG_JOBS);
            break;
        }

        /* Strip trailing newline and carriage return characters */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        /* Preserve raw command entered by the user for jobs output (Guideline 4) */
        strncpy(raw_cmd, line, sizeof(raw_cmd) - 1);
        raw_cmd[sizeof(raw_cmd) - 1] = '\0';

        /* Tokenize input by whitespace (spaces and tabs) */
        int argc = 0;
        char *token = strtok(line, " \t");
        while (token != NULL && argc < MAX_ARGS) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");
        }
        argv[argc] = NULL;

        /* Guideline 10: An empty command (enter / spaces) is not an error */
        if (argc == 0) {
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        /* Guideline 5: Detect background command indicated by '&' */
        int is_bg = 0;
        size_t last_len = strlen(argv[argc - 1]);
        if (strcmp(argv[argc - 1], "&") == 0) {
            is_bg = 1;
            argv[argc - 1] = NULL;
            argc--;
        } else if (last_len > 0 && argv[argc - 1][last_len - 1] == '&') {
            is_bg = 1;
            argv[argc - 1][last_len - 1] = '\0';
            if (strlen(argv[argc - 1]) == 0) {
                argv[argc - 1] = NULL;
                argc--;
            }
        }

        /* Handle edge case where input was solely '&' */
        if (argc == 0) {
            printf("hw1shell: invalid command\n");
            fflush(stdout);
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        /* Guideline 2: Internal command 'exit' */
        if (strcmp(argv[0], "exit") == 0) {
            handle_exit(bg_jobs, MAX_BG_JOBS);
        }

        /* Guideline 3: Internal command 'cd' */
        if (strcmp(argv[0], "cd") == 0) {
            handle_cd(argc, argv);
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        /* Guideline 4: Internal command 'jobs' */
        if (strcmp(argv[0], "jobs") == 0) {
            handle_jobs(bg_jobs, MAX_BG_JOBS);
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        /* External Command Handling */
        int bg_slot = -1;
        if (is_bg) {
            bg_slot = find_free_bg_slot(bg_jobs, MAX_BG_JOBS);
            if (bg_slot == -1) {
                /* Guideline 6: Max 4 background commands reached */
                printf("hw1shell: too many background commands running\n");
                fflush(stdout);
                reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
                continue;
            }
        }

        /* Flush output buffers before fork to prevent duplicate output in pipes */
        fflush(stdout);
        fflush(stderr);

        /* Guideline 7: Fork process to execute external command */
        pid_t pid = fork();
        if (pid < 0) {
            int saved_errno = errno;
            print_syscall_error("fork", saved_errno);
        } else if (pid == 0) {
            /* Child process: execute command via execvp */
            execvp(argv[0], argv);

            /* If execvp returns, it failed - Guidelines 11 & 13 */
            int saved_errno = errno;
            printf("hw1shell: invalid command\n");
            print_syscall_error("execvp", saved_errno);
            fflush(stdout);
            fflush(stderr);
            _exit(1);
        } else {
            /* Parent process */
            if (!is_bg) {
                /* Guideline 8: Foreground command - block until child completes */
                int status;
                pid_t res = waitpid(pid, &status, 0);
                if (res == -1) {
                    int saved_errno = errno;
                    print_syscall_error("waitpid", saved_errno);
                }
            } else {
                /* Guideline 9: Background command - record in table and report start */
                bg_jobs[bg_slot].pid = pid;
                strncpy(bg_jobs[bg_slot].cmd, raw_cmd, sizeof(bg_jobs[bg_slot].cmd) - 1);
                bg_jobs[bg_slot].cmd[sizeof(bg_jobs[bg_slot].cmd) - 1] = '\0';
                printf("hw1shell: pid %d started\n", (int)pid);
                fflush(stdout);
            }
        }

        /* Guideline 12: Reap finished background children at end of iteration */
        reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
    }

    return 0;
}
