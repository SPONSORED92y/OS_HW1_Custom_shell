// gcc bash_execute.c bash_read_line.c bash_split_line.c my_shell.c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

struct FD
{
    int flags;
    int pip1[2];
    int pip2[2];
    int copy_stdin;
    int copy_stdout;
    char in[30];
    char out[30];
    int file_in;
    int file_out;
} fd;

void initialize_fd()
{
    if (pipe(fd.pip1) < 0)
    // pipe[0] read fd[2]
    // pipe[1] write fd[3]
    {
        fprintf(stderr, "bash: pipe error\n");
    }
    fd.copy_stdin = dup(0);       // fd[5]
    fd.copy_stdout = dup(1);      // fd[6]
    if (strcmp(fd.in, "\0") != 0) // fd[7]
    {
        fd.file_in = open(fd.in, O_RDONLY);
        close(STDIN_FILENO);
        dup2(fd.file_in, 0); // stdin now covered by in_file
    }
    else
    {
        fd.file_in = 0;
    }
    if (strcmp(fd.out, "\0") != 0) // fd[8]
    {
        fd.file_out = open(fd.out, O_WRONLY | O_CREAT | O_TRUNC, 777);
    }
    else
    {
        fd.file_out = 0;
    }
}

int main()
{
    char *args[] = {"ls", "-l", "|", "grep", ".c", 0};
    // set flags
    fd.flags = 0;
    int pip_count = 0; //|
    int seen = 0;      // temp

    // check <|>
    for (int i = 0; strcmp(args[i], "\0") != 0; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            flags += 4;
            strcpy(fd.in, args[i + 1]);
            // delete and move NULL forward
            for (int j = i + 2;; j++)
            {
                strcpy(args[j - 2], args[j]);
                if (strcmp(args[j], "\0") == 0)
                {
                    strcpy(args[j - 2], "\0");
                    strcpy(args[j - 1], "\0");
                    break;
                }
            }
            i--;
        }
        else if (strcmp(args[i], "|") == 0)
        {
            if (pip_count == 0)
            {
                flags += 2;
            }
            pip_count++;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            flags += 1;
            strcpy(fd.out, args[i + 1]);
            // delete and move NULL forward
            for (int j = i + 2;; j++)
            {
                args[j - 2] = args[j];
                if (strcmp(args[j], "\0") == 0)
                {
                    strcpy(args[j - 2], "\0");
                    strcpy(args[j - 1], "\0");
                    break;
                }
            }
            i--;
        }
    }

    initialize_fd();

    // adjust IO
    if (fd.flags == 1 || fd.flags == 5)
    {
        close(STDOUT_FILENO);
        dup2(fd.file_out, 1);
    }
    else if (fd.flags == 2 || fd.flags == 3 || fd.flags == 6 || fd.flags == 7)
    {
        dup2(fd.pip[1], 1);
        close()
    }
    // first cmd

    if (bash_launch(args) == 0)
    {
        return 0;
    }

    // adjust IO after first cmd
    if (fd.flags == 2 || fd.flags == 3 || fd.flags == 6 || fd.flags == 7)
    {
        close(STDIN_FILENO);
        dup2(fd.pip[0], 0);
    }
    // follow up cmds
    for (int i = 1;; i++)
    {
        if (strcmp(args[i], "\0") == 0)
        {
            break;
        }
        close(fd.pip1[1]); // close old write
        if (pipe(fd.pip2) < 0)
        {
            fprintf(stderr, "bash: pipe error\n");
        }
        dup2(fd.pip2[1], 1);//cover stdout with new erite

        if (strcmp(args[i], "|") == 0)
        {
            i++;
            pip_count--;
            // adjust IO before last cmd
            if (pip_count == 0)
            {
                if (fd.flags == 2 || fd.flags == 6)
                {
                    close(fd.pip[1]);
                    dup2(fd.copy_stdout, 1);
                }
                else if (fd.flags == 3 || fd.flags == 7)
                {
                    close(fd.pip[1]);
                    dup2(fd.file_out, 1);
                }
            }else{
                
            }

            if (bash_launch(args + i) == 0)
            {
                return 0;
            }
        }
    }
    // restore IO
    dup2(fd.copy_stdin, 0);
    dup2(fd.copy_stdout, 1);
    close(fd.pip[0]);
    close(fd.pip[1]);
    close(fd.copy_stdin);
    close(fd.copy_stdout);
    if (fd.file_in > 0)
    {
        close(fd.file_in);
    }
    if (fd.file_out > 0)
    {
        close(fd.file_out);
    }
    if (pid == 0)
    {
        exit(0);
    }
    return 1;
}

int bash_launch(char **args)
{
    int len = 0;
    for (len = 0;; len++)
    {
        if (strcmp(args[len], "\0") == 0 || strcmp(args[len], "|") == 0)
        {
            break;
        }
    }
    char **new_args = malloc(sizeof(char *) * (len + 1));
    for (int i = 0; i < len; i++)
    {
        new_args[i] = malloc(sizeof(char) * 50);
    }
    for (int i = 0; i < len; i++)
    {
        strcpy(new_args[i], args[i]);
    }
    new_args[len] = NULL;
    //
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0)
    {
        // child
        // fflush(stdout);
        if (execvp(new_args[0], new_args) == -1)
        {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // error forking
        perror("lsh");
    }
    else
    {
        //  parent
        close_fd();
        wpid = waitpid(pid, &status, WUNTRACED);
        restore_fd();
    }
    free(new_args);
    return 1;
}