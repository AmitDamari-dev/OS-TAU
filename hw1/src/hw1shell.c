/*
 * hw1shell.c - a simple shell.
 * Internal commands: cd, jobs, exit. Everything else runs via fork/execvp.
 * Up to 4 background jobs (commands ending with &) at a time.
 */

#include "hw1shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

void print_prompt(void) {
    printf(SHELL_PROMPT);
    fflush(stdout);
}

void print_syscall_error(const char *syscall_name, int err) {
    printf("hw1shell: %s failed, errno is %d\n", syscall_name, err);
    fflush(stdout);
}

/*
 * cd must be internal: a child process has its own working directory, so a
 * chdir in a child would not change the directory of the shell itself.
 */
int handle_cd(int argc, char **argv) {
    /* Only "cd <dir>" is valid; any other form is an invalid command */
    if (argc != 2) {
        printf("hw1shell: invalid command\n");
        fflush(stdout);
        return -1;
    }

    if (chdir(argv[1]) == -1) {
        int saved_errno = errno;
        printf("hw1shell: invalid command\n");
        print_syscall_error("chdir", saved_errno);
        return -1;
    }

    return 0;
}

void handle_jobs(const bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid > 0) {
            printf("%d\t%s\n", (int)jobs[i].pid, jobs[i].cmd);
        }
    }
    fflush(stdout);
}

void handle_exit(bg_job_t *jobs, int max_jobs) {
    /* Wait for every running background job so none is left as a zombie */
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

void reap_finished_bg_jobs(bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid > 0) {
            int status;
            /* WNOHANG: return immediately if the job is still running */
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

int find_free_bg_slot(const bg_job_t *jobs, int max_jobs) {
    for (int i = 0; i < max_jobs; i++) {
        if (jobs[i].pid == 0) {
            return i;
        }
    }
    return -1;
}

int main(void) {
    bg_job_t bg_jobs[MAX_BG_JOBS];
    memset(bg_jobs, 0, sizeof(bg_jobs));

    char line[MAX_LINE_LEN + 1];
    char raw_cmd[MAX_LINE_LEN + 1];
    char *argv[MAX_ARGS + 1];

    /* Each iteration: prompt, read, parse, run, then reap finished bg jobs */
    while (1) {
        print_prompt();

        /* EOF (Ctrl+D) behaves like exit */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            handle_exit(bg_jobs, MAX_BG_JOBS);
            break;
        }

        /* Strip the trailing newline */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        /* Keep the original text for the jobs listing; strtok modifies line */
        strncpy(raw_cmd, line, sizeof(raw_cmd) - 1);
        raw_cmd[sizeof(raw_cmd) - 1] = '\0';

        /* Split into arguments by spaces and tabs */
        int argc = 0;
        char *token = strtok(line, " \t");
        while (token != NULL && argc < MAX_ARGS) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");
        }
        argv[argc] = NULL;

        /* An empty line is not an error */
        if (argc == 0) {
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        /* '&' may be a separate token ("sleep 1 &") or attached ("sleep 1&") */
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

        /* The line was only '&' */
        if (argc == 0) {
            printf("hw1shell: invalid command\n");
            fflush(stdout);
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        if (strcmp(argv[0], "exit") == 0) {
            handle_exit(bg_jobs, MAX_BG_JOBS);
        }

        if (strcmp(argv[0], "cd") == 0) {
            handle_cd(argc, argv);
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        if (strcmp(argv[0], "jobs") == 0) {
            handle_jobs(bg_jobs, MAX_BG_JOBS);
            reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
            continue;
        }

        /* External command. For a background one, make sure a slot is free */
        int bg_slot = -1;
        if (is_bg) {
            bg_slot = find_free_bg_slot(bg_jobs, MAX_BG_JOBS);
            if (bg_slot == -1) {
                printf("hw1shell: too many background commands running\n");
                fflush(stdout);
                reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
                continue;
            }
        }

        /* Flush before fork so the child doesn't duplicate buffered output */
        fflush(stdout);
        fflush(stderr);

        pid_t pid = fork();
        if (pid < 0) {
            int saved_errno = errno;
            print_syscall_error("fork", saved_errno);
        } else if (pid == 0) {
            /* Child: execvp only returns if it failed */
            execvp(argv[0], argv);

            int saved_errno = errno;
            printf("hw1shell: invalid command\n");
            print_syscall_error("execvp", saved_errno);
            fflush(stdout);
            fflush(stderr);
            /* _exit, not exit: don't flush the parent's copied stdio buffers again */
            _exit(1);
        } else {
            if (!is_bg) {
                /* Foreground: wait for the child to finish */
                int status;
                pid_t res = waitpid(pid, &status, 0);
                if (res == -1) {
                    int saved_errno = errno;
                    print_syscall_error("waitpid", saved_errno);
                }
            } else {
                /* Background: remember it and report its pid */
                bg_jobs[bg_slot].pid = pid;
                strncpy(bg_jobs[bg_slot].cmd, raw_cmd, sizeof(bg_jobs[bg_slot].cmd) - 1);
                bg_jobs[bg_slot].cmd[sizeof(bg_jobs[bg_slot].cmd) - 1] = '\0';
                printf("hw1shell: pid %d started\n", (int)pid);
                fflush(stdout);
            }
        }

        reap_finished_bg_jobs(bg_jobs, MAX_BG_JOBS);
    }

    return 0;
}
