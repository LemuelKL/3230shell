#include <sys/resource.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void close_fds(int fd[][2], int n)
{
    for (int i = 0; i < n; i++)
    {
        close(fd[i][0]);
        close(fd[i][1]);
    }
}

void pipe_expr(int fd[][2], int i, char **args, int pipe_cnt, int expr_cnt, int *expr_idx)
{
    if (i == 0) // first expr
    {
        dup2(fd[i][1], STDOUT_FILENO);
    }
    else if (i == expr_cnt - 1) // last expr
    {
        dup2(fd[i - 1][0], STDIN_FILENO);
    }
    else // middle exprs
    {
        dup2(fd[i - 1][0], STDIN_FILENO);
        dup2(fd[i][1], STDOUT_FILENO);
    }
    close_fds(fd, pipe_cnt);
    if (execvp(args[expr_idx[i]], args + expr_idx[i]) == -1)
        printf("3230shell: '%s': %s\n", args[expr_idx[i]], strerror(errno));
    exit(0); // just return to main shell loop
}

void run_pipe(char **args, int tk_cnt, bool timex_md)
{
    // EXPR_1 | EXPR_2 | ... | EXPR_N
    int pipe_cnt = 0;
    int expr_idx[tk_cnt]; // array of pointers to the start of each expression
    expr_idx[0] = 0;      // first expression starts at args[0]
    for (int i = 0; i < tk_cnt; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            pipe_cnt++;
            expr_idx[pipe_cnt] = i + 1;
            args[i] = NULL; // replace pipe symbol with NULL
        }
    }
    int expr_cnt = pipe_cnt + 1;
    int fd[expr_cnt][2];
    for (int i = 0; i < pipe_cnt; i++)
    {
        if (pipe(fd[i]) == -1)
        {
            printf("3230shell: pipe error\n");
            exit(0);
        }
    }
    int child_pids[expr_cnt];
    for (int i = 0; i < expr_cnt; i++)
    {
        int pid = fork();
        if (pid == -1)
        {
            printf("3230shell: pipe error\n");
            exit(0);
        }
        if (pid == 0)
        {
            pipe_expr(fd, i, args, pipe_cnt, expr_cnt, expr_idx);
        }
        child_pids[i] = pid;
    }
    // parents have no role to play, close all
    close_fds(fd, pipe_cnt);
    for (int i = 0; i < expr_cnt; i++)
    {
        int status;
        struct rusage rusage;
        wait4(child_pids[i], &status, 0, &rusage);
        if (timex_md)
        {
            printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n",
                   child_pids[i],
                   args[expr_idx[i]],
                   rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0,
                   rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
        }
    }
    exit(0);
}