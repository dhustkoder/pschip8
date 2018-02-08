SRC_FILES=src/*.c src/sdl/*.c
HEADER_FIELS=src/*.h src/sdl/*.h

CC=gcc
CFLAGS=-std=gnu99 -Wall -O3 -Isrc/ -Isrc/sdl -DPLATFORM_LINUX $(shell sdl-config --cflags)


all: $(SRC_FILES) $(HEADER_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o linux/pschip8 $(shell sdl-config --libs)

