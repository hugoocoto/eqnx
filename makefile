FLAGS = -Wall -Wextra -I.

all: plugin.so plugin2.so main 
	./main

main: main.o makefile
	gcc $(FLAGS) main.o -o main -rdynamic

main.o: main.c makefile minicoro.h
	gcc $(FLAGS) -c main.c -o main.o 

plugin.so: plugin.c makefile main.c
	gcc $(FLAGS) plugin.c -o plugin.so -shared

plugin2.so: plugin2.c makefile main.c
	gcc $(FLAGS) plugin2.c -o plugin2.so -shared

minicoro.h:
	wget https://raw.githubusercontent.com/edubart/minicoro/refs/heads/main/minicoro.h
