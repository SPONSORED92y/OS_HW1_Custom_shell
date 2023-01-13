// the declarations are too messy.
// since no multiple include would happen, I combined .c and .h to avoid some troubles
#ifndef builtinG
#define builtinG
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int bash_help(char **args);
int bash_cd(char **args);
int bash_echo(char **args);
int bash_exit(char **args);
int bash_record(char **args);
int bash_mypid(char **args);
char ***get_record_buffer();

char *builtin_str[] = {
    "help",
    "cd",
    "echo",
    "exit",
    "record",
    "mypid"};

int (*builtin_func[])(char **) = {
    &bash_help,
    &bash_cd,
    &bash_echo,
    &bash_exit,
    &bash_record,
    &bash_mypid};

int bash_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}
// builtin function implementations
int bash_help(char **args)
{
    printf("----------------------------------------------------------\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");
    printf("1: help:\tshow all built in function info\n");
    printf("2: cd:\tchange directory\n");
    printf("3: echo:\techo the strings to standard output\n");
    printf("4: record:\tshow last 16 cmds you typed in\n");
    printf("5: replay:\tre-execute the cmd showed in record\n");
    printf("6: mypid:\tfind and print process ids\n");
    printf("7: ecit:\texit shell\n");
    printf("----------------------------------------------------------\n");
    return 1;
}

int bash_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "bash: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("bash");
        }
    }
    return 1;
}

int bash_echo(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "bash: expected argument to \"echo\"\n");
    }
    else if (strcmp(args[1], "-n") == 0)
    {
        for (int i = 2;; i++)
        {
            if (args[i] == NULL || strcmp(args[i], "|") == 0)
            {
                break;
            }
            printf("%s", args[i]);
            printf(" ");
        }
    }
    else
    {
        for (int i = 1;; i++)
        {
            if (args[i] == NULL || strcmp(args[i], "|") == 0)
            {
                break;
            }
            printf("%s", args[i]);
            printf(" ");
        }
        printf("\n");
    }
    return 1;
}

int bash_exit(char **args)
{
    printf("------------------------\n");
    printf("SPONSORED's shell: Bye!\n");
    printf("------------------------\n");
    return 0;
}

int bash_record(char **args)
{
    printf("----------------------------------------------------------\n");
    printf("History comands:\n");
    for (int i = 0; i < 16; i++)
    {
        printf("%d", i + 1);
        printf(":");
        if (get_record_buffer()[i] == NULL)
        {
            printf("\n");
            continue;
        }
        for (int j = 0;; j++)
        {
            if (get_record_buffer()[i][j] == NULL)
            {
                printf("\n");
                break;
            }
            printf(" ");
            printf("%s", get_record_buffer()[i][j]);
        }
    }
    printf("----------------------------------------------------------\n");
    return 1;
}

int bash_mypid(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "bash: expected argument to \"mypid\"\n");
    }
    else if (strcmp(args[1], "-i") == 0)
    {
        printf("%d", getpid());
        printf("\n");
    }
    else if (strcmp(args[1], "-p") == 0)
    {
        if (args[2] == NULL)
        {
            fprintf(stderr, "bash: expected argument to \"mypid -p\"\n");
            return 1;
        }
        FILE *pfile;
        char *status = "/status";
        char path[50] = "/proc/";
        strncat(path, args[2], strlen(args[2]));
        strncat(path, status, strlen(status));
        printf("%s", path);
        printf("\n");
        pfile = fopen(path, "r");
        if (pfile == NULL)
        {
            fprintf(stderr, "bash: error open proc status\n");
            return 1;
        }
        char str[100];
        char *result = NULL;
        while ((fgets(str, 100, pfile)) != NULL)
        {
            // Find first occurrence of word in str
            result = strstr(str, "PPid");
            if (result != NULL)
            {
                break;
            }
        }
        result = result + 6;
        printf("%s", result);
        fclose(pfile);
    }
    else if (strcmp(args[1], "-c") == 0)
    {
        if (args[2] == NULL)
        {
            fprintf(stderr, "bash: expected argument to \"mypid -p\"\n");
            return 1;
        }
        char **ls_cmd = malloc(sizeof(char *) * 3);
        ls_cmd[0] = "ls";
        ls_cmd[1] = "/proc/";
        ls_cmd[2] = NULL;
        int ls_pip[2];
        if (pipe(ls_pip) < 0)
        {
            fprintf(stderr, "bash: pipe error\n");
        }
        int copy_stdout = dup(1);
        dup2(ls_pip[1], 1);
        pid_t pid, wpid;
        int status;
        pid = fork();
        if (pid == 0)
        {
            // child
            if (execvp(ls_cmd[0], ls_cmd) == -1)
            {
                perror("lsh");
            }
            exit(EXIT_FAILURE);
        }
        else
        {
            // parent
            wpid = waitpid(pid, &status, WUNTRACED);
        }
        free(ls_cmd);
        fflush(stdout);
        dup2(copy_stdout, 1);
        char ls[3000];
        read(ls_pip[0], ls, 3000);
        close(ls_pip[0]);
        close(ls_pip[1]);
        int position = 0;
        char **tokens = malloc(sizeof(char *) * 300); // null terminated
        char *t;
        if (!tokens)
        {
            fprintf(stderr, "bash: allocation error\n");
            exit(EXIT_FAILURE);
        }
        t = strtok(ls, "\n");
        while (t != NULL)
        {
            if (t[0] > 48 && t[0] < 58)
            {
                tokens[position] = t;
                position++;
            }
            t = strtok(NULL, "\n");
        }
        tokens[position] = NULL;
        for (int i = 0;; i++)
        {
            if (tokens[i] == NULL)
            {
                break;
            }
            else
            {
                FILE *pfile;
                char *status = "/status";
                char path[50] = "/proc/";
                strncat(path, tokens[i], strlen(tokens[i]));
                strncat(path, status, strlen(status));
                pfile = fopen(path, "r");
                if (pfile == NULL)
                {
                    continue; // we ignore process that has exited above(which called ls)
                }
                char str[100];
                char *result = NULL;
                while ((fgets(str, 100, pfile)) != NULL)
                {
                    // Find first occurrence of word in str
                    result = strstr(str, "PPid");
                    if (result != NULL)
                    {
                        break;
                    }
                }
                result = result + 6; // remove "PPis:    "
                // remove the trailing '\n'
                char copy_result[20];
                strcpy(copy_result, result);
                for (int j = 0;; j++)
                {
                    if (copy_result[j] == '\n')
                    {
                        copy_result[j] = 0;
                        break;
                    }
                }
                if (strcmp(args[2], copy_result) == 0)
                {
                    printf("%s", tokens[i]);
                    printf("\n");
                }
                fclose(pfile);
            }
        }
        free(tokens);
    }
    else
    {
        fprintf(stderr, "bash: invalid argument to \"mypid\"\n");
    }
    return 1;
}

#endif
