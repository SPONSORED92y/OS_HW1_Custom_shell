#include "bash_read_line.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *bash_read_line()
{
    int buffer_size = 1024;
    int position = 0;
    char *buffer = malloc(sizeof(char) * buffer_size);
    int c;
    if (!buffer)
    {
        fprintf(stderr, "bash: allocation error\n");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        c = getchar();
        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            // strcpy(buffer + position, &c);
            buffer[position] = c;
        }
        position++;
        if (position >= buffer_size)
        {
            buffer_size += 1024;
            buffer = realloc(buffer, buffer_size);
            if (!buffer)
            {
                fprintf(stderr, "bash: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}