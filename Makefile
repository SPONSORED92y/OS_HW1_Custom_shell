CC = gcc
FLAGS = -Wall -std=c11

all: my_shell

my_shell: 
	$(CC) $(FLAGS) -o my_shell my_shell.c bash_execute.c bash_read_line.c bash_split_line.c 

.PHONY: clean
clean_exc:
	rm -f my_shell
clean:
	rm -f *.o 