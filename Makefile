

all: default

default: chrono

chrono: chrono.o
	gcc -Wall -Werror -lncurses -lpthread chrono.o -o chrono

chrono.o: chrono.c
