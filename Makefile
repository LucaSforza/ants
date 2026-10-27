all: main

build:
	mkdir -p build

main: main.c build
	gcc -Wall -Wextra main.c -I./raylib-6.0/build/raylib/include/ -L./raylib-6.0/build/raylib/ -lraylib -lX11 -lm  -o build/ant
