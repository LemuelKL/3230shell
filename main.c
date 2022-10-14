#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

void show_prompt()
{
    printf("$$ 3230shell ##  ");
}

void read_command(char *command[])
{
    char line[1024];
    const char delim[2] = " ";
    char *token;
    scanf("%99[^\n]", line);
    token = strtok(line, delim);

    int i = 0;
    while (token != NULL)
    {
        token = strtok(NULL, delim);
        strcpy(command[i], token);
        i++;
    }
}

int main(int argc, char **argv)
{
    char *command[30];
    while (1)
    {
        show_prompt();
        read_command(command);
        printf("command: %s", command[0]);
        if (fork() == 0) // child
        {
            execlp("ls", "ls", "-l", NULL);
        }
        else // parent
        {
            wait(NULL); // wait for child to finish
        }
    }
    return 0;
}