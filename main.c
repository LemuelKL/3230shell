// Lee Kwok Lam
// 3035782231
// Developed on Windows 11 with WSL2 Ubuntu 20.04

// The shell is the root parent.
// Each prompt is a new child of the shell
// Each command is a new child of the prompt, i.e. the grandchild of the shell

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

static int main_pid;

void sigint_handler(int signum)
{
    int pid = getpid();
    int ppid = getppid();
    if (pid == main_pid) // main shell process uninterruptable
        return;
    else if (ppid == main_pid) // prompt process
    {
        printf("\n");
        exit(0);
    }
    else
    {
        printf("!!!!!");
    }
}

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

int interactive_prompt()
{
    char *cmd[MAX_WORD_NUM + 1];

    printf("$$ 3230shell ##  ");
    int cmd_cnt = read_command(cmd);
    cmd[cmd_cnt] = NULL;

    for (int i = 0; i < cmd_cnt; i++)
    {
        if (strcmp("&", cmd[i]) == 0 && (i != cmd_cnt - 1 || i == 0))
        {
            printf("3230shell: '&' should not appear in the middle of the command line\n");
            return 1;
        }
    }

    bool timeX = false;
    if (strcmp(cmd[0], "") == 0)
    {
        return 0;
    }
    if (strcmp(cmd[0], "exit") == 0)
    {
        if (cmd_cnt != 1)
        {
            printf("3230shell: 'exit' has too many arguments\n");
            return 1;
        }
        return 100;
    }
    if (strcmp(cmd[0], "timeX") == 0)
    {
        if (cmd_cnt == 1)
        {
            printf("3230shell: 'timeX' cannot be a standalone command\n");
            return 1;
        }
        if (strcmp(cmd[cmd_cnt - 1], "&") == 0)
        {
            printf("3230shell: 'timeX' cannot be run in background mode\n");
            return 1;
        }
        timeX = true;
    }
    bool handle_signals = (signal(SIGINT, SIG_IGN) != SIG_IGN); // check if previous handler is SIG_IGN
    pid_t pid = fork();
    if (pid == 0)
    {
        if (handle_signals)
            signal(SIGINT, SIG_DFL); // purge SIGINT handler
        int exec_status;
        if (timeX)
            exec_status = execvp(cmd[1], cmd + 1);
        else
            exec_status = execvp(cmd[0], cmd);
        if (exec_status == -1)
            printf("3230shell: '%s': %s\n", cmd[0], strerror(errno));
    }
    else if (pid != -1)
    {
        int status;
        int corpse = waitpid(pid, &status, 0);
        if (WIFSIGNALED(status))
            printf("%s\n", strsignal(WTERMSIG(status)));

        if (timeX)
        {
            struct rusage rusage;
            int ret = wait4(pid, &status, 0, &rusage);
            printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n",
                   pid,
                   cmd[1],
                   rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0,
                   rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
        }
    }

    return 0;
}

void start_child(int signum)
{
    return;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    int run = 1;
    while (run)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            printf("Error: fork failed");
        }
        else if (pid == 0) // child
        {
            signal(SIGUSR1, start_child);
            pause();
            int x = interactive_prompt();
            exit(x);
        }
        else // parent
        {
            main_pid = getpid();
            kill(pid, SIGUSR1);

            int status;
            int child_pid = waitpid(pid, &status, WUNTRACED);
            int exit_status = WEXITSTATUS(status);
            switch (exit_status)
            {
            case 0:
                // prompt exists normally, let's spawn a new one
                break;
            case 1:
                // prompt (not command) exits with general error,
                // probably due to invalid command, let's spawn a new one
                break;
            case 100:
                printf("3230shell: exiting\n");
                run = 0;
                break;

            default:
                printf("3230shell: unknown error code %d\n", exit_status);
                break;
            }
        }
    }
    return 0;
}