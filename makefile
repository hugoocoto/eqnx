FLAGS = -Wall -Wextra -I. -ggdb

all: plugin.so plugin2.so main window.c test_window
	# ./main

main: main.o makefile
	gcc $(FLAGS) main.o -o main -rdynamic

main.o: main.c makefile minicoro.h
	gcc $(FLAGS) -c main.c -o main.o 

plugin.so: plugin.c makefile main.c
	gcc $(FLAGS) plugin.c -o plugin.so -shared

plugin2.so: plugin2.c makefile main.c
	gcc $(FLAGS) plugin2.c -o plugin2.so -shared

test_window: window.o
	gcc $(FLAGS) window.o -o test_window

window.o: window.c makefile 
	gcc $(FLAGS) -c window.c -o window.o 

minicoro.h:
	wget https://raw.githubusercontent.com/edubart/minicoro/refs/heads/main/minicoro.h
