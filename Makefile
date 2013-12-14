# A makefile for my LD28 game


C99 = gcc -std=c99 -Wall -Werror -pedantic
LIBS = -lGL -lGLU -lglfw3 -lm -lX11 -lXxf86vm -lXrandr -lpthread -lXi

all: YOGO

YOGO: main.o
	$(C99) main.o -g $(LIBS) -o YOGO

main.o: main.c
	$(C99) -g -c main.c

clean:
	rm -f main.o YOGO