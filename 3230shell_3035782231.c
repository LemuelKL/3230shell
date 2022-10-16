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
// * All the bullet points in the grading criteria table

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

static int prompt_pid = -1;
static int shell_pid = -1;
void sigint_handler(int sig)
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

static char *proc_name[32768]; // shared by shell loop parent and its sigchld handler
void sigchld_handler(int sig)
{
    int status;
    struct rusage rusage;
    pid_t dead_child = wait3(&status, WNOHANG, &rusage);

    char *comm = proc_name[dead_child];

    if (dead_child == -1)
    {
        return;
    }
    else // background process
    {
        while (waitpid(prompt_pid, NULL, WNOHANG) == 0) // to wait for the current prompt to finish
        {
            usleep(1);
        }
        if (proc_name[dead_child] == NULL) // when the user ^C or entered nothing to the prompt
        {
            return;
        }
        // since the previous prompt must have finished,
        // and the next prompt is not started yet (still stuck in this handler),
        // the followings will be printed before the next prompt
        if (WIFEXITED(status))
        {
            printf("[%d] %s Done\n", dead_child, comm);
        }
        else if (WIFSIGNALED(status))
        {
            printf("[%d] %s %s\n", dead_child, comm, strsignal(WTERMSIG(status)));
        }
    }
}

volatile sig_atomic_t execute = 0;
void sigusr1_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        prompt_pid = getpid();
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

bool is_bg_md(char **args, int tk_cnt)
{
    for (int i = 0; i < tk_cnt; i++)
    {
        if (strcmp(args[i], "&") == 0)
        {
            if (i != tk_cnt - 1)
            {
                printf("3230shell: '&' must be the last token\n");
                exit(0);
            }
            return true;
        }
    }
    return false;
}

void exec_cmd(char *cmd, int tk_cnt, char *raw, char **args, bool timex_md)
{
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
        exit(0); // just return to main shell loop
    }
}

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
        int bg_fd[2];
        pipe(bg_fd); // for child (prompt) to notify parent (shell) that the
                     // child will be running in background
        int cmd_fd[2];
        pipe(cmd_fd); // transfer cmd from child (prompt) to parent (shell)
        pid = fork();
        if (pid == 0)
        {
            while (!execute)
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
            bool bg_md = is_bg_md(args, tk_cnt);
            if (bg_md)
            {
                args[tk_cnt - 1] = NULL;
                tk_cnt--;
            }

            close(bg_fd[0]);
            write(bg_fd[1], &bg_md, sizeof(bool));
            close(bg_fd[1]);

            close(cmd_fd[0]);
            write(cmd_fd[1], cmd, MAX_LINE_LEN + 1);
            close(cmd_fd[1]);

            exec_cmd(cmd, tk_cnt, raw, args, timex_md);
            exit(0);
        }
        else if (pid > 0)
        {
            shell_pid = getpid(); // set shell_pid before child runs
            prompt_pid = pid;     // set prompt_pid before child runs
            kill(pid, SIGUSR1);

            close(bg_fd[1]);
            bool bg_md;
            read(bg_fd[0], &bg_md, sizeof(bool));
            close(bg_fd[0]);

            close(cmd_fd[1]);
            char r_cmd[MAX_LINE_LEN + 1];
            read(cmd_fd[0], r_cmd, MAX_LINE_LEN + 1);
            close(cmd_fd[0]);
            if (bg_md)
            {
                proc_name[pid] = r_cmd;
            }

            // printf("(P) bg_md: %d\n", bg_md);
            if (!bg_md)
            {
                int status;
                int dead_child = waitpid(pid, &status, WUNTRACED);
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
    }
    return 0;
}