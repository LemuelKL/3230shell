// Lee Kwok Lam
// 3035782231
// Developed on Windows 11 with WSL2 Ubuntu 20.04

///// OVERVIEW /////
// The shell is the root parent.
// Each prompt is a new child of the shell
// Each command run as the prompt process

#include <sys/resource.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE_LEN 1024
#define MAX_WORD_NUM 30

int read_command(char *cmd[30])
{
    char ch;
    char word[MAX_WORD_NUM + 1][MAX_LINE_LEN + 1];
    int i = 0, j = 0, k = 0;
    while (i < MAX_LINE_LEN - 1)
    {
        ch = getchar();
        if (ch == ' ')
        {
            word[j][k] = '\0';
            cmd[j] = word[j];
            j++;
            k = 0;
        }
        else if (ch == '\n')
        {
            word[j][k] = '\0';
            cmd[j] = word[j];
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
    // printf("[SIGUSR1] %d\n", getpid());
    if (sig == SIGUSR1)
    {
        shell_pid = getppid();
        execute = 1;
    }
}

void close_fd(int fd[][2], int i, int total)
{
}

void list_fd(char str[], int fd[][2], int end_idx_incl)
{
    for (int i = 0; i <= end_idx_incl; i++)
    {
        printf("%s fd[%d][0]: %d\n", str, i, fcntl(fd[i][0], F_GETFD));
        printf("%s fd[%d][1]: %d\n", str, i, fcntl(fd[i][1], F_GETFD));
    }
}
int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGUSR1, sigusr1_handler);
    char *cmd[MAX_WORD_NUM + 1];
    pid_t pid;
    while (1)
    {
        pid = fork();
        if (pid == 0)
        {
            while (!execute)
            {
                pause();
            }
            printf("$$ 3230shell ## ");
            int cmd_cnt = read_command(cmd);
            cmd[cmd_cnt] = NULL;

            if (strcmp(cmd[0], "") == 0)
                exit(0);
            if (strcmp(cmd[0], "exit") == 0)
            {
                if (cmd_cnt != 1)
                {
                    printf("3230shell: 'exit' has too many arguments\n");
                    exit(0);
                }
                exit(100);
            }
            if (strcmp(cmd[0], "timeX") == 0)
            {
                if (cmd_cnt < 2)
                {
                    printf("3230shell: 'timeX' cannot be a stnadalone command\n");
                    exit(0);
                }
                if (strcmp(cmd[cmd_cnt - 1], "&") == 0)
                {
                    printf("3230shell: 'timeX' cannot be run in background mode\n");
                    exit(0);
                }

                int timeX_pid = fork();
                if (timeX_pid == 0)
                {
                    if (execvp(cmd[1], cmd + 1) == -1)
                        printf("3230shell: '%s': %s\n", cmd[1], strerror(errno));
                    exit(0);
                }
                else
                {
                    int status;
                    struct rusage rusage;
                    wait4(timeX_pid, &status, 0, &rusage);
                    printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n",
                           timeX_pid,
                           cmd[1],
                           rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0,
                           rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
                    exit(0);
                }
            }
            // PIPE
            bool is_pipe = false;
            for (int i = 0; i < cmd_cnt; i++)
            {
                if (strcmp(cmd[i], "|") == 0)
                {
                    is_pipe = true;
                    break;
                }
            }
            if (is_pipe)
            {
                for (int i = 0; i < cmd_cnt; i++)
                {
                    if (strcmp(cmd[i], "|") == 0 && (i == 0 || i == cmd_cnt - 1))
                    {
                        printf("3230shell: | cannot be the head or tail of the command\n");
                        exit(0);
                    }
                    if (strcmp(cmd[i], "|") == 0 && strcmp(cmd[i + 1], "|") == 0)
                    {
                        printf("3230shell: should not have two consectuive | without in-between command\n");
                        exit(0);
                    }
                }
                // real logic starts here
                int pipe_sym_cnt = 0;
                int cmd_idx[cmd_cnt];
                cmd_idx[0] = 0;
                for (int i = 0; i < cmd_cnt; i++)
                {
                    if (strcmp(cmd[i], "|") == 0)
                    {
                        pipe_sym_cnt++;
                        cmd_idx[pipe_sym_cnt] = i + 1;

                        cmd[i] = NULL;
                    }
                }
                if (pipe_sym_cnt == 0)
                    exit(0);
                int cmd_cnt = pipe_sym_cnt + 1;
                int pipe_cnt = cmd_cnt - 1;
                int fd[pipe_cnt][2];
                for (int i = 0; i < pipe_cnt; i++)
                {
                    if (pipe(fd[i]) == -1)
                    {
                        printf("3230shell: pipe error\n");
                        exit(0);
                    }
                }
                int child_pids[cmd_cnt];
                for (int i = 0; i < cmd_cnt; i++)
                {
                    int pid = fork();
                    if (pid == 0)
                    {
                        // printf("i:%d  (PID):%d  (PPID):%d\n", i, getpid(), getppid());
                        if (i == 0) // first cmd
                        {
                            close(fd[i][0]); // first cmd does not read

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

                            execvp(cmd[cmd_idx[i]], cmd + cmd_idx[i]);
                        }
                        else if (i == cmd_cnt - 1) // last cmd
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

                            execvp(cmd[cmd_idx[i]], cmd + cmd_idx[i]);
                        }
                        else // middle cmds
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

                            execvp(cmd[cmd_idx[i]], cmd + cmd_idx[i]);
                        }
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
                for (int i = 0; i < cmd_cnt; i++)
                {
                    close(fd[i][0]);
                    close(fd[i][1]);
                }
                for (int i = 0; i < cmd_cnt; i++)
                {
                    int status;
                    // printf("wait for child %d\n", child_pids[i]);
                    int child_id = waitpid(child_pids[i], &status, 0);
                    // printf("%d exitted with %d\n", child_id, WEXITSTATUS(status));
                }
                exit(0); // safety net
            }
            if (execvp(cmd[0], cmd) == -1)
                printf("3230shell: '%s': %s\n", cmd[0], strerror(errno));
            exit(0);
        }
        else if (pid > 0)
        {
            shell_pid = getpid();
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