

#include <sys/resource.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void pipe_expr(int fd[][2], int i, char **args, int pipe_cnt, int expr_cnt, int *expr_idx)
{
    if (i == 0) // first expr
    {
        close(fd[i][0]); // first expr does not read

        // close all other fds[j], j != 0
        for (int j = 1; j < pipe_cnt; j++)
        {
            close(fd[j][0]);
            close(fd[j][1]);
        }

        // redirect stdout to fd[0][1]
        // data entrance of entire pipe line
        dup2(fd[i][1], STDOUT_FILENO);
        close(fd[i][1]);

        execvp(args[expr_idx[i]], args + expr_idx[i]);
    }
    else if (i == expr_cnt - 1) // last expr
    {
        for (int j = 0; j < pipe_cnt; j++)
        {
            close(fd[j][1]);
            if (j != i - 1)
                close(fd[j][0]);
        }

        // only read from fd[i-1][0]
        dup2(fd[i - 1][0], STDIN_FILENO);
        close(fd[i - 1][0]);

        execvp(args[expr_idx[i]], args + expr_idx[i]);
    }
    else // middle exprs
    {
        for (int j = 0; j < pipe_cnt; j++)
        {
            if (j != i - 1)
                close(fd[j][0]);
            if (j != i)
                close(fd[j][1]);
        }

        // only read from fd[i-1][0]
        dup2(fd[i - 1][0], STDIN_FILENO);
        close(fd[i - 1][0]);

        // only write to fd[i][1]
        dup2(fd[i][1], STDOUT_FILENO);
        close(fd[i][1]);

        execvp(args[expr_idx[i]], args + expr_idx[i]);
    }
}

void run_pipe(char **args, int tk_cnt, bool timex_md)
{
    // EXPR_1 | EXPR_2 | ... | EXPR_N
    int pipe_cnt = 0;
    int expr_idx[tk_cnt]; // array of pointers to the start of each expression
    expr_idx[0] = 0; // first expression starts at args[0]
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
    for (int i = 0; i < expr_cnt; i++)
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
        if (pid == 0)
        {
            pipe_expr(fd, i, args, pipe_cnt, expr_cnt, expr_idx);
        }
        else
        {
            child_pids[i] = pid;
        }
    }
    // leftmost of the pipes is none
    // WRITE_END of 1st pipe is written by 1st child
    // rightmost of the pipes is STDOUT
    // parents have no role to play, close all
    for (int i = 0; i < expr_cnt; i++)
    {
        close(fd[i][0]);
        close(fd[i][1]);
    }
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