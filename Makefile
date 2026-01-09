# Makefile for building sdl_2.cpp
# Requires SDL3 installed under /usr/local.

CC = g++
# Add libtiff include and library path
CFLAGS = -I/usr/local/include/SDL3 -I../tiff-4.5.0/libtiff
LDFLAGS = -L/usr/local/lib -L../tiff_build/libtiff -lSDL3 -ltiff

TARGETS = sdl_3
SRCS = sdl_3.cpp climate_wind.cpp climate_maps.cpp helper.cpp rivers.cpp terrain_noise.cpp config.cpp textures_core.cpp textures_maps.cpp data_generation.cpp tiff_write.cpp profiler.cpp

all: $(TARGETS)

$(TARGETS): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
