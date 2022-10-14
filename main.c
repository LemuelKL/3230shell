#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void sigint_handler(int signum)
{
    return;
}

void show_prompt()
{
    static int first_time = 1;
    if (first_time)
    {
        // printf("%s", " \e[1;1H\e[2J");
        printf("$$ 3230shell ##  ");
        first_time = 0;
    }
    else
    {
        printf("\n$$ 3230shell ##  ");
    }
}

void read_command(char *command[30])
{
    const char delim[2] = " ";
    char line[1024];
    scanf(" %99[^\n]", line);

    int i = 0;
    command[i] = strtok(line, delim);
    while (command[i] != NULL)
    {
        command[++i] = strtok(NULL, delim);
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);

    char *command[30];
    while (1)
    {
        show_prompt();
        read_command(command);
        int pid = fork();
        if (pid == -1)
        {
            printf("Error: fork failed");
        }
        else if (pid == 0) // child
        {
            int exec_status = execvp(command[0], command);
            if (errno == ENOENT)
                printf("3230shell: '%s': No such file or directory", command[0]);
            if (errno == EACCES)
                printf("3230shell: '%s': Permission denied", command[0]);
        }
        else // parent
        {
            wait(NULL);
        }
    }
    return 0;
}