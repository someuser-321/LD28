#!/bin/sh

gcc main.c -g -std=c99 -Wall -Werror -pedantic -lGL -lGLU -lglfw3 -lm -lX11 -lXxf86vm -lXrandr -lpthread -lXi -o main