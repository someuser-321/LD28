# A makefile for my LD28 game


C99 = gcc -std=c99 -Wall -Werror -pedantic
LIBS = -lGL -lGLU -lglfw3 -lm -lX11 -lXxf86vm -lXrandr -lpthread -lXi

all: yogo

yogo: yogo.o
	$(C99) yogo.o -g $(LIBS) -o yogo

yogo.o: yogo.c
	$(C99) -g -c yogo.c

clean:
	rm -f yogo.o yogo