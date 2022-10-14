#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>

#define MAX_LINE_LEN 1024
#define MAX_WORD_NUM 30

static int prompt_pid;

void sigint_handler(int signum)
{
    kill(prompt_pid, SIGKILL);
    printf("\n");
    return;
}

void show_prompt()
{
    static int first_time = 1;
    if (first_time)
    {
        // printf("%s", " \e[1;1H\e[2J");
        first_time = 0;
    }
    printf("$$ 3230shell ##  ");
}

int read_command(char *command[30])
{
    char ch;
    char word[MAX_WORD_NUM + 1][MAX_LINE_LEN + 1];
    int i = 0, j = 0, k = 0;
    while (i < MAX_LINE_LEN - 1)
    {
        ch = getchar();
        // int scan_status = scanf("%c", &ch);
        if (ch == ' ')
        {
            word[j][k] = '\0';
            command[j] = word[j];
            j++;
            k = 0;
        }
        else if (ch == '\n')
        {
            word[j][k] = '\0';
            command[j] = word[j];
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

void interactive_prompt()
{
    char *command[MAX_WORD_NUM + 1];
    while (1)
    {
        show_prompt();
        int cmd_cnt = read_command(command);
        command[cmd_cnt] = NULL;
        int pid = fork();
        if (pid == -1)
        {
            printf("Error: fork failed");
        }
        else if (pid == 0) // child
        {
            int exec_status = execvp(command[0], command);
            if (errno == ENOENT)
                printf("3230shell: '%s': No such file or directory\n", command[0]);
            if (errno == EACCES)
                printf("3230shell: '%s': Permission denied\n", command[0]);
        }
        else // parent
        {
            wait(NULL);
        }
    }
}
int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    while (1)
    {
        int pid = fork();
        if (pid == -1)
        {
            printf("Error: fork failed");
        }
        else if (pid == 0) // child
        {
            interactive_prompt();
        }
        else // parent
        {
            prompt_pid = pid;
            wait(NULL);
        }
    }

    return 0;
}