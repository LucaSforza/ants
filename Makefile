CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -D_DEFAULT_SOURCE -I.

all: main

build:
	mkdir -p build

RAYLIB_DIR := raylib-6.0
RAYLIB_BUILD_DIR := $(RAYLIB_DIR)/build
RAYLIB_LIBRARY := $(RAYLIB_BUILD_DIR)/raylib/libraylib.a

raylib:
	cmake -S $(RAYLIB_DIR) -B $(RAYLIB_BUILD_DIR) -DBUILD_EXAMPLES=OFF
	cmake --build $(RAYLIB_BUILD_DIR) --target raylib

SOURCES := main.c discrete.c continuous.c continuous_cli.c continuous_ui.c
HEADERS := continuous.h continuous_app.h

main: build/ant

build/ant: $(SOURCES) $(HEADERS) $(RAYLIB_LIBRARY) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -I$(RAYLIB_BUILD_DIR)/raylib/include -L$(RAYLIB_BUILD_DIR)/raylib -lraylib -lX11 -lm -o $@

headless: build/ant-headless

build/ant-headless: continuous.c continuous_cli.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DANT_HEADLESS continuous.c continuous_cli.c -lm -o $@

build/test-continuous: tests/test_continuous.c continuous.c continuous.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_continuous.c continuous.c -lm -o $@

build/test-discrete: tests/test_discrete.c discrete.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_discrete.c -o $@

test: build/test-continuous build/test-discrete build/ant-headless
	./build/test-continuous
	./build/test-discrete
	python3 tests/test_cli.py

sanitize: | build
	$(CC) $(CPPFLAGS) -std=c11 -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_continuous.c continuous.c -lm -o build/test-sanitize
	./build/test-sanitize
	$(CC) $(CPPFLAGS) -std=c11 -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_discrete.c -o build/test-discrete-sanitize
	./build/test-discrete-sanitize
	$(CC) $(CPPFLAGS) -std=c11 -Wall -Wextra -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -DANT_HEADLESS continuous.c continuous_cli.c -lm -o build/ant-headless-sanitize
	ANT_BINARY=build/ant-headless-sanitize python3 tests/test_cli.py

experiments: build/ant-headless
	python3 experiments/run.py

latex: | build
	pdflatex -interaction=nonstopmode -halt-on-error -output-directory=build docs/model.tex
	pdflatex -interaction=nonstopmode -halt-on-error -output-directory=build docs/model.tex

$(RAYLIB_LIBRARY): $(shell find $(RAYLIB_DIR)/src -type f \( -name '*.c' -o -name '*.h' \)) $(RAYLIB_DIR)/CMakeLists.txt
	cmake -S $(RAYLIB_DIR) -B $(RAYLIB_BUILD_DIR) -DBUILD_EXAMPLES=OFF
	cmake --build $(RAYLIB_BUILD_DIR) --target raylib

.PHONY: all main headless build raylib test sanitize experiments latex
