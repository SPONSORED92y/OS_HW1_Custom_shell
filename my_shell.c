#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bash_read_line.h"
#include "bash_split_line.h"
#include "bash_execute.h"
void shell_loop();
char ***get_record_buffer();
char ***record_buffer;
// int bash_record(char **args);
int main()
{
    // initiallize
    record_buffer = calloc(sizeof(char **) * 16, sizeof(char **));
    for (int i = 0; i < 16; i++)
    {
        record_buffer[i] = calloc(sizeof(char *) * 30, sizeof(char *));
        for (int j = 0; j < 30; j++)
        {
            record_buffer[i][j] = calloc(sizeof(char) * 50, sizeof(char));
        }
    }
    printf("============================\n");
    printf("welcome to SPONSORED's shell\n");
    printf("============================\n");
    shell_loop();
    // free
    for (int i = 0; i < 30; i++)
    {
        for (int j = 0; j < 50; j++)
        {
            free(record_buffer[i][j]);
        }
        free(record_buffer[i]);
    }
    free(record_buffer);
    //

    return 0;
}

void shell_loop()
{
    int status = 1;
    do
    {
        printf(">>> $");
        char *lines = bash_read_line();
        char **args = bash_split_line(lines);
        free(lines);
        if (args != NULL)
        {
            status = bash_execute(args);
        }
        free(args);
    } while (status == 1);
}

char ***get_record_buffer()
{
    return record_buffer;
}