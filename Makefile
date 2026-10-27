all: main

build:
	mkdir -p build

RAYLIB_DIR := raylib-6.0
RAYLIB_BUILD_DIR := $(RAYLIB_DIR)/build
RAYLIB_LIBRARY := $(RAYLIB_BUILD_DIR)/raylib/libraylib.a

raylib:
	cmake -S $(RAYLIB_DIR) -B $(RAYLIB_BUILD_DIR) -DBUILD_EXAMPLES=OFF
	cmake --build $(RAYLIB_BUILD_DIR) --target raylib

main: main.c build $(RAYLIB_LIBRARY)
	gcc -Wall -Wextra main.c -I./raylib-6.0/build/raylib/include/ -L./raylib-6.0/build/raylib/ -lraylib -lX11 -lm  -o build/ant

$(RAYLIB_LIBRARY): $(shell find $(RAYLIB_DIR)/src -type f \( -name '*.c' -o -name '*.h' \)) $(RAYLIB_DIR)/CMakeLists.txt
	cmake -S $(RAYLIB_DIR) -B $(RAYLIB_BUILD_DIR) -DBUILD_EXAMPLES=OFF
	cmake --build $(RAYLIB_BUILD_DIR) --target raylib

.PHONY: all build raylib
