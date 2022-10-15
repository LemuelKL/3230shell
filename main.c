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

    for (int j = 0; j < total; j++)
    {
        if (i == j)
        {
            close(fd[j][1]);
            continue;
        }
        if (i + 1 == j)
        {
            close(fd[j][0]);
            continue;
        }
        close(fd[j][0]);
        close(fd[j][1]);
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
                for (int i = 0; i < cmd_cnt; i++)
                {
                    if (strcmp(cmd[i], "|") == 0)
                    {
                        pipe_sym_cnt++;
                    }
                }
                if (pipe_sym_cnt == 0)
                    exit(0);
                int cmd_cnt = pipe_sym_cnt + 1;
                int pipe_cnt = cmd_cnt + 1;
                int fd[pipe_cnt][2];
                for (int i = 0; i < pipe_cnt; i++)
                {
                    if (pipe(fd[i]) == -1)
                    {
                        printf("3230shell: pipe error\n");
                        exit(0);
                    }
                }
                // Data written to the write end of a pipe can be read from the read end of the pipe.
                // printf("cmd_cnt: %d\n", cmd_cnt);
                // printf("pipe_cnt: %d\n", pipe_cnt);
                for (int i = 0; i < cmd_cnt; i++)
                {
                    int pid = fork();
                    if (pid == 0)
                    {
                        printf("i:%d  child %d\n", i, getpid());
                        if (i == 0)
                        {
                            close(fd[0][0]);
                            close(fd[0][1]);
                            dup2(fd[1][1], STDOUT_FILENO);
                            close(fd[1][1]);
                            close(fd[1][0]);
                            close(fd[2][0]);
                            close(fd[2][1]);
                            execlp("ls", "ls", "-l", NULL);
                        }
                        if (i == 1) // 1,0 2,1
                        {
                            close(fd[0][0]);
                            close(fd[0][1]);
                            close(fd[1][1]);
                            close(fd[2][0]);
                            dup2(fd[1][0], STDIN_FILENO);
                            close(fd[1][0]);
                            dup2(fd[2][1], STDOUT_FILENO);
                            close(fd[2][1]);
                            execlp("grep", "grep", "main", NULL);
                        }
                    }
                }
                close(fd[0][0]);
                close(fd[0][1]);
                close(fd[1][0]);
                close(fd[1][1]);
                close(fd[2][1]); // close the WRITE end of the pipe
                char buf[1024 * 1024];
                int b_read = 0;
                int n = -2;
                while (n != 0)
                {
                    n = read(fd[2][0], buf + b_read, 1);
                    b_read += n;
                }
                buf[b_read] = '\0';
                printf("%s", buf);
                close(fd[2][0]);
                int status;
                int child_id = waitpid(pid, &status, 0);
                printf("child_id: %d exit: %d\n", child_id, WEXITSTATUS(status));
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