# Compiler and flags
CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lm -lpthread
LDFLAGS_LINUX = -lm -ldl -lpthread

# Source files
SRC = main.c quickjs.c quickjs-libc.c libregexp.c libunicode.c libbf.c cutils.c custom_js_module_loader.c
SRC_LINUX = main-linux.c quickjs-linux.c quickjs-libc.c libregexp.c libunicode.c libbf.c cutils.c custom_js_module_loader.c
OBJ = $(SRC:.c=.o)

# Executable names
EXE = bin/pfmjs
# EXE_MAC = bin/pfmjs.dmg
EXE_WIN = bin/pfmjs.exe

# Default target for current OS
all: $(EXE)

# Build for Linux
bird:
	$(CC) $(CFLAGS) -o $(EXE) $(SRC_LINUX) $(LDFLAGS)

# Build for Windows (cross-compile)
doors:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o $(EXE_WIN) $(SRC) -lws2_32 -static

# macaroni:
# 	$(CC) $(CFLAGS) -o $(EXE_MAC) $(SRC) $(LDFLAGS)

# Clean
clean:
	rm -f $(EXE) $(EXE_WIN) *.o
