#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LEN 1024
#define MAX_WORD_NUM 30

static int rage_cnt = 0;
static int main_pid;
static int prompt_pid;

void sigint_handler(int signum)
{
    return;
}

void show_prompt()
{
    printf("$$ 3230shell ##  ");
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

    show_prompt();
    int cmd_cnt = read_command(cmd);
    cmd[cmd_cnt] = NULL;

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
            printf("3230shell: 'timeX' cannot be a standalone command");
            return 1;
        }
    }
    int exec_status = execvp(cmd[0], cmd);
    if (errno == ENOENT)
        printf("3230shell: '%s': No such file or directory\n", cmd[0]);
    if (errno == EACCES)
        printf("3230shell: '%s': Permission denied\n", cmd[0]);

    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    while (1)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            printf("Error: fork failed");
        }
        else if (pid == 0) // child
        {
            raise(SIGSTOP);
            int x = interactive_prompt();
            exit(x);
        }
        else // parent
        {
            waitpid(pid, NULL, WUNTRACED);
            prompt_pid = pid;
            main_pid = getpid();
            kill(pid, SIGCONT);

            int status;
            int child_pid = wait(&status);
            int exit_status = WEXITSTATUS(status);
            printf("child [%d] returns: %d\n", child_pid, exit_status);
            switch (exit_status)
            {
            case 0:
                break;
            case 1:
                break;
            case 15:
                break;
            case 100:
                printf("3230shell: exiting\n");
                run = 0;
                break;

            default:
                break;
            }
        }
    }
    return 0;
}