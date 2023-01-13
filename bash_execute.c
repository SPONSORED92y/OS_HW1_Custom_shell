#include "bash_execute.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
char ***get_record_buffer();

struct FD
{
    int pip[2];
    int copy_stdin;
    int copy_stdout;
    char in[30];
    char out[30];
    int file_in;
    int file_out;
} fd;

void initialize_fd()
{
    if (pipe(fd.pip) < 0)
    // pipe[0] read fd[2]
    // pipe[1] write fd[3]
    {
        fprintf(stderr, "bash: pipe error\n");
    }
    fd.copy_pip[0] = dup(fd.pip[0]);
    fd.copy_pip[1] = dup(fd.pip[1]);
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
void close_fd()
{
    close(fd.pip[0]);
    close(fd.pip[1]);
    if (fd.file_in > 0)
    {
        close(fd.file_in);
    }
}

void restore_fd()
{

    fd.pip[0] = dup(fd.copy_pip[0]);
    fd.pip[1] = dup(fd.copy_pip[1]);
    if (strcmp(fd.in, "\0") != 0) // fd[7]
    {
        fd.file_in = open(fd.in, O_RDONLY);
    }
    /*if (fd.table[0] == 2) // pipe read
    {
        fd.pip[0] = dup(fd.copy_pip[0]);
        dup2(pip[0], 0);
    }*/
}

int bash_execute(char **args)
{
    // check empty comand
    if (strcmp(args[0], "\0") == 0)
    {
        return 1;
    }
    // set flags
    int flags = 0;     //<|>
    int amps = 0;      //&
    int pip_count = 0; //|
    int seen = 0;      // temp
    pid_t pid = -1;
    // check &
    for (int i = 0; strcmp(args[i], "\0") != 0; i++)
    {
        if (strcmp(args[i], "&") == 0)
        {
            amps = 1;
            strcpy(args[i], "\0");
            break;
        }
    }
    if (amps == 1)
    {
        int status;
        pid = fork();
        if (pid > 0)
        {
            // parent
            printf("[Pid]: %d\n", pid);
            return 1;
        }
        else if (pid < 0)
        {
            // error forking
            perror("lsh: error forking");
        }
        // else { child process }
    }
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
    // check empty comand
    if (strcmp(args[0], "\0") == 0 || strcmp(args[0], "|") == 0)
    {
        fprintf(stderr, "bash: invalid comands\n");
        if (pid == 0)
        {
            exit(0);
        }
        return 1;
    }
    // adjust IO
    if (flags == 1 || flags == 5)
    {
        close(STDOUT_FILENO);
        dup2(fd.file_out, 1);
    }
    else if (flags == 2 || flags == 3 || flags == 6 || flags == 7)
    {
        dup2(fd.pip[1], 1);
    }
    // first cmd
    seen = 0;
    // printf("arg[0] is:%s\n", args[0]);
    for (int j = 0; j < bash_num_builtins(); j++)
    {
        if (strcmp(args[0], builtin_str[j]) == 0)
        {
            if ((*builtin_func[j])(args) == 0)
            {
                // fflush(stdout);
                return 0;
            }
            seen = 1;
            break;
        }
    }
    if (seen == 0)
    {
        if (bash_launch(args) == 0)
        {
            return 0;
        }
    }
    // adjust IO after first cmd
    if (flags == 2 || flags == 3 || flags == 6 || flags == 7)
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
        else if (strcmp(args[i], "|") == 0)
        {
            i++;
            pip_count--;
            // adjust IO before last cmd
            if (pip_count == 0)
            {
                if (flags == 2 || flags == 6)
                {
                    close(fd.pip[1]);
                    dup2(fd.copy_stdout, 1);
                }
                else if (flags == 3 || flags == 7)
                {
                    close(fd.pip[1]);
                    dup2(fd.file_out, 1);
                }
            }
            seen = 0;
            for (int j = 0; j < bash_num_builtins(); j++)
            {
                if (strcmp(args[i], builtin_str[j]) == 0)
                {
                    if ((*builtin_func[j])(args + i) == 0)
                    {
                        return 0;
                    }
                    seen = 1;
                    break;
                }
            }
            if (seen == 0)
            {
                if (bash_launch(args + i) == 0)
                {
                    return 0;
                }
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