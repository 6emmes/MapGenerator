# Makefile for building sdl_2.cpp
# Requires SDL3 installed under /usr/local.

CC = g++
CFLAGS = -I/usr/local/include/SDL3
LDFLAGS = -L/usr/local/lib -lSDL3

TARGETS = sdl_3
SRCS = sdl_3.cpp texture.cpp terrain.cpp bmp_write.cpp

all: $(TARGETS)

$(TARGETS): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
