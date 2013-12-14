# A makefile for my LD28 game


C99 = gcc -std=c99 -Wall -Werror -pedantic
LIBS = -lGL -lGLU -lglfw3 -lm -lX11 -lXxf86vm -lXrandr -lpthread -lXi

all: yogo

yogo: main.o game.o
	$(C99) main.o game.o $(LIBS) -o yogo

main.o: main.c
	$(C99) -c main.c
game.o: game.c
	$(C99) -c game.c 
