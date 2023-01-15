#include "bash_execute.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
char ***get_record_buffer();

#define READ 0
#define WRITE 1

int bash_execute(char **args)
{

    // check empty comand
    if (strcmp(args[0], "\0") == 0)
    {
        return 1;
    }
    // set flags
    int pip_prev[2]; // read from
    int pip_cur[2];  // write to
    int copy_stdin;
    int copy_stdout;
    char in[30] = "\0";
    char out[30] = "\0";
    int first_input;     // could be stdin or file in
    int last_output;     // could be stdout or file out
    int amps = 0;        //&
    int cmd_count = 0;   //|
    int ran_builtin = 0; // temp
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
            strcpy(in, args[i + 1]);
            // delete and move NULL forward
            for (int j = i + 2;; j++)
            {
                strcpy(args[j - 2], args[j]);
                if (strcmp(args[j], "\0") == 0)
                {
                    strcpy(args[j - 1], "\0");
                    break;
                }
            }
            i--;
        }
        else if (strcmp(args[i], "|") == 0)
        {
            cmd_count++;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            strcpy(out, args[i + 1]);
            // delete and move NULL forward
            for (int j = i + 2;; j++)
            {
                args[j - 2] = args[j];
                if (strcmp(args[j], "\0") == 0)
                {
                    strcpy(args[j - 1], "\0");
                    break;
                }
            }
            i--;
        }
    }

    cmd_count++; // cmd_count = pip count+1

    if ((copy_stdin = dup(STDIN_FILENO)) < 0)
    {
        perror("bash: ");
        exit(EXIT_FAILURE);
    }
    if ((copy_stdout = dup(STDOUT_FILENO)) < 0)
    {
        perror("bash: ");
        exit(EXIT_FAILURE);
    }
    // first_input: determine stdin or file in
    if (strcmp(in, "\0") != 0)
    {
        if ((first_input = open(in, O_RDONLY)) < 0)
        {
            perror("bash: ");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        first_input = dup(STDIN_FILENO);
    }
    // first_output: determine stdout or file out
    if (strcmp(out, "\0") != 0)
    {
        fprintf(stderr, "err135\n");
        if ((last_output = open(out, O_WRONLY | O_CREAT | O_TRUNC, 777)) < 0)
        {
            perror("bash: ");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "err143\n");
        last_output = dup(STDOUT_FILENO);
    }

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
    int iteration = 0;
    // int p3[2];
    while (1)
    {
        fprintf(stderr, "it: %d %d\n", iteration, cmd_count);
        fflush(stderr);
        if (iteration == 0) // first cmd
        {
        }
        else if (strcmp(args[iteration], "|") == 0) // following cmd starts with |
        {
            iteration++;
        }
        else // not a cmd (could be opt)
        {
            iteration++;
            continue;
        }
        //---------------------
        //        |
        // input  v
        if (iteration == 0)
        { // first cmd
          // could be stdin or file_in
            dup2(first_input, STDIN_FILENO);
            close(first_input);
        }
        else
        {
            if ((dup2(pip_cur[READ], STDIN_FILENO)) < 0)
            {
                perror("lll: ");
                exit(EXIT_FAILURE);
            }
            if (close(pip_cur[READ]) < 0)
            {
                perror("close1: ");
                exit(EXIT_FAILURE);
            }
            // close(pip_cur[READ]);
            //  pipe
            /*pip_prev[READ] = dup(pip_cur[READ]);
            close(pip_cur[READ]);
            dup2(pip_prev[READ], STDIN_FILENO);
            close(pip_prev[READ]);
            */
        }
        //---------------------
        //         |
        // output  v

        if (cmd_count - 1 == 0)
        { // last cmd
            // could be stdout or file_out
            if ((dup2(last_output, STDOUT_FILENO)) < 0)
            {
                perror("bash: ");
                fflush(stderr);
                exit(EXIT_FAILURE);
            }
            close(last_output);
        }
        else
        {
            // pipe
            // pip_cur[0] = 0;
            // pip_cur[1] = 0;
            if (pipe(pip_cur) < 0)
            {
                perror("shell: ");
            }
            if ((dup2(pip_cur[WRITE], STDOUT_FILENO)) < 0)
            {
                perror("kkk: ");
                exit(EXIT_FAILURE);
            }
            // dup2(pip_cur[WRITE], STDOUT_FILENO);
            // close(pip_cur[WRITE]);
            if (close(pip_cur[WRITE]) < 0)
            {
                perror("close: ");
                exit(EXIT_FAILURE);
            }
        }
        //---------------------
        // execute cmd
        // check if it's builtin
        ran_builtin = 0;
        for (int j = 0; j < bash_num_builtins(); j++)
        {
            if (strcmp(args[iteration], builtin_str[j]) == 0)
            {
                if ((*builtin_func[j])(args + iteration) == 0)
                {
                    if (pid == 0)
                    {
                        exit(0);
                    }
                    return 0;
                }
                fflush(stdout);
                ran_builtin = 1;
                break;
            }
        }
        if (ran_builtin == 0) // if the cmd is not builtin
        {
            if (bash_launch(args + iteration) == 0)
            {
                if (pid == 0)
                {
                    exit(0);
                }
                return 0;
            }
            fflush(stdout);
        }

        iteration++;
        cmd_count--;
        if (cmd_count == 0)
        {
            break;
        }
    }
    //  restore IO
    if ((dup2(copy_stdin, STDIN_FILENO)) < 0)
    {
        perror("bash: ");
        exit(EXIT_FAILURE);
    }
    if ((dup2(copy_stdout, STDOUT_FILENO)) < 0)
    {
        perror("bash: ");
        exit(EXIT_FAILURE);
    }
    close(copy_stdin);
    close(copy_stdout);
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
    fprintf(stderr, "%s\n", new_args[0]);
    // printf("SSSSSSSSSSSSSSSSSSS\n");
    fflush(stderr);
    // fflush(stdout);
    //
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0)
    {
        // child
        if (execvp(new_args[0], new_args) == -1)
        {
            perror("bash: ");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // error forking
        perror("bash: ");
    }
    else
    {
        //  parent
        close(STDOUT_FILENO);
        // close(pip_cur[WRITE]);
        wpid = waitpid(pid, &status, WUNTRACED);
    }
    free(new_args);
    return 1;
}
