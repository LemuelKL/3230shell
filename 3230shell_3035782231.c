// Lee Kwok Lam
// 3035782231
// Developed on Windows 11 with WSL2 Ubuntu 20.04
//
///// OVERVIEW /////
// * The shell is the root parent.
// * Each prompt is a new child of the shell
// * Each command run as the prompt process (replace)
// * For pipping, the shell forks a child for each
//   EXPR: EXPR_1 | EXPR_2 | ... | EXPR_N
// * Pipping supports arbitrary number of expressions that could have arbitrary
//   number of arguments
// * For timeX, a grandchild to the shell is forked to run the command, 
//   the middle child waits for the grandchild to finish, then displays the
//   statistics, and then exit back to main shell.
//
///// COMPLETED FEATURES /////
// * All non-bonus features

#include <sys/resource.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE_LEN 1024
#define MAX_WORD_NUM 30

int read_command(char cmd[MAX_LINE_LEN + 1], char *args[MAX_WORD_NUM + 1], char raw[MAX_LINE_LEN + 1])
{
    char ch;
    char word[MAX_WORD_NUM + 1][MAX_LINE_LEN + 1];
    int i = 0, j = 0, k = 0;
    while (i < MAX_LINE_LEN - 1)
    {
        ch = getchar();
        raw[i] = ch;
        if (ch == ' ')
        {
            word[j][k] = '\0';
            args[j] = word[j];
            j++;
            k = 0;
        }
        else if (ch == '\n')
        {
            word[j][k] = '\0';
            args[j] = word[j];
            strcpy(cmd, word[0]); // as per the standard
            raw[i] = '\0';        // remove the newline and terminate the string
            return j + 1;
        }
        else
        {
            word[j][k] = ch;
            k++;
        }
        i++;
    }
}

static int shell_pid = -1;
void sigint_handler(int signum)
{
    int pid = getpid();
    int ppid = getppid();
    if (pid == shell_pid)
    {
        return;
    }
    else if (ppid == shell_pid)
    {
        printf("\n");
        exit(0);
    }
}

void sigchld_handler(int sig)
{
    int status;
    pid_t pid = wait(&status);
}

volatile sig_atomic_t execute = 0;
void sigusr1_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        shell_pid = getppid();
        execute = 1;
    }
}

void timeX(char *cmd, char **args)
{
    int timeX_pid = fork();
    if (timeX_pid == 0)
    {
        if (execvp(cmd, args) == -1)
            printf("3230shell: '%s': %s\n", cmd, strerror(errno));
        exit(0);
    }
    else
    {
        int status;
        struct rusage rusage;
        wait4(timeX_pid, &status, 0, &rusage);
        printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n",
               timeX_pid,
               cmd,
               rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0,
               rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
        exit(0);
    }
}

void handle_empty(char *cmd)
{
    if (strcmp(cmd, "") == 0)
        exit(0);
}

void handle_exit(char *cmd, int tk_cnt)
{
    if (strcmp(cmd, "exit") == 0)
    {
        if (tk_cnt != 1)
        {
            printf("3230shell: 'exit' has too many arguments\n");
            exit(0);
        }
        exit(100);
    }
}

bool handle_timex(char *cmd, int tk_cnt, char *raw)
{
    if (strcmp(cmd, "timeX") == 0)
    {
        if (tk_cnt < 2)
        {
            printf("3230shell: 'timeX' cannot be a stnadalone command\n");
            exit(0);
        }
        if (raw[strlen(raw) - 1] == '&')
        {
            printf("3230shell: 'timeX' cannot be run in background\n");
            exit(0);
        }
        return true;
    }
    return false;
}

bool has_pipe_sym(char *raw)
{
    for (int i = 0; i < strlen(raw); i++)
    {
        if (raw[i] == '|')
            return true;
    }
    return false;
}

void vld_pipe(char **args, int tk_cnt)
{
    if (strcmp(args[0], "|") == 0)
    {
        printf("3230shell: '|' cannot be a standalone command\n");
        exit(0);
    }
    if (strcmp(args[tk_cnt - 1], "|") == 0)
    {
        printf("3230shell: '|' cannot be the last token\n");
        exit(0);
    }
    for (int i = 0; i < tk_cnt - 1; i++)
    {
        if (strcmp(args[i], "|") == 0 && strcmp(args[i + 1], "|") == 0)
        {
            printf("3230shell: should not have two consecutive | without in-between command\n");
            exit(0);
        }
    }
}

void run_pipe(char **args, int tk_cnt, bool timex_md);

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGUSR1, sigusr1_handler);
    char cmd[MAX_LINE_LEN + 1];
    char *args[MAX_WORD_NUM + 1];
    pid_t pid;
    while (1)
    {
        pid = fork();
        if (pid == 0)
        {
            while (!execute) // wait for SIGUSR1
            {
                pause();
            }
            printf("$$ 3230shell ## ");
            char raw[MAX_LINE_LEN + 1];
            int tk_cnt = read_command(cmd, args, raw);
            args[tk_cnt] = NULL;
            handle_empty(cmd);
            handle_exit(cmd, tk_cnt);
            bool timex_md = handle_timex(cmd, tk_cnt, raw);
            bool pipe_md = has_pipe_sym(raw);
            if (pipe_md)
                vld_pipe(args, tk_cnt);
            if (timex_md && !pipe_md)
                timeX(*(args + 1), args + 1);
            else if (!timex_md && pipe_md)
                run_pipe(args, tk_cnt, false);
            else if (timex_md && pipe_md)
                run_pipe(args + 1, tk_cnt - 1, true);
            else
            {
                int exec_status = execvp(cmd, args);
                if (exec_status == -1)
                    printf("3230shell: '%s': %s\n", cmd, strerror(errno));
            }
        }
        else if (pid > 0)
        {
            shell_pid = getpid(); // set shell_pid before child runs
            kill(pid, SIGUSR1);
            int status;
            int dead_child = waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_code = WEXITSTATUS(status);
                if (exit_code == 100)
                {
                    printf("Bye bye!\n");
                    return 0;
                }
            }
            else if (WIFSIGNALED(status))
                printf("%s\n", strsignal(status));
        }
    }
    return 0;
}