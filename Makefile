# -*- MakeFile -*-

all: smallsh

smallsh: main.o input_funcs.o shell_process.o
	gcc --std=gnu99 -g -o smallsh main.o input_funcs.o shell_process.o

main.o: main.c input_funcs.h command_info.h shell_process.h list_node.h
	gcc --std=gnu99 -c -g main.c

input_funcs.o: input_funcs.c input_funcs.h command_info.h
	gcc --std=gnu99 -c -g input_funcs.c

shell_process.o: shell_process.c shell_process.h command_info.h list_node.h
	gcc --std=gnu99 -c -g shell_process.c

clean:
	rm -f *.o smallsh

