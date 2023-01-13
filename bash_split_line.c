#include "bash_split_line.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
char ***get_record_buffer();
char **bash_split_line(char *line)
{
    int position = 0;
    char **tokens = calloc(sizeof(char *) * 30, sizeof(char *));
    for (int i = 0; i < 30; i++)
    {
        tokens[i] = calloc(sizeof(char) * 50, sizeof(char));
    }
    char *t;
    if (!tokens)
    {
        fprintf(stderr, "bash: allocation error\n");
        exit(EXIT_FAILURE);
    }
    t = strtok(line, " ");
    while (t != NULL)
    {
        if (strcmp(t, "replay") == 0)
        {
            t = strtok(NULL, " ");
            int n = 0;
            if (t != NULL && sscanf(t, "%d", &n) == 1 && (n >= 1 && n <= 16) && get_record_buffer()[n - 1] != NULL)
            {
                for (int j = 0; strcmp(get_record_buffer()[n - 1][j], "\0") != 0; j++)
                {
                    strcpy(tokens[position], get_record_buffer()[n - 1][j]);
                    position++;
                }
            }
            else
            {
                perror("replay: wrong args");
                return NULL;
            }
        }
        else
        {
            strcpy(tokens[position], t);
            position++;
        }
        t = strtok(NULL, " ");
    }
    for (int i = position; i < 30; i++)
    {
        strcpy(tokens[i], "\0");
    }
    // update record
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 30; j++)
        {

            if (i == 15)
            {
                if (strcmp(tokens[j], "\0") != 0)
                {
                    strcpy(get_record_buffer()[i][j], tokens[j]);
                }
                else
                {
                    strcpy(get_record_buffer()[i][j], "\0");
                }
            }
            else
            {
                if (get_record_buffer()[i + 1][j] != NULL)
                {
                    strcpy(get_record_buffer()[i][j], get_record_buffer()[i + 1][j]);
                }
                else
                {
                    strcpy(get_record_buffer()[i][j], "\0");
                }
            }
        }
    }
    return tokens;
}
