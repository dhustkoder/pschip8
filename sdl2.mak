SRC_FILES=src/*.c src/sdl2/*.c
HEADER_FIELS=src/*.h src/sdl2/*.h

CC=gcc
CFLAGS=-std=gnu99 -Wall -O2 -g -Isrc/ -Isrc/sdl2 $(shell sdl2-config --cflags) -DPLATFORM_SDL2


all: $(SRC_FILES) $(HEADER_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o sdl2/pschip8 $(shell sdl2-config --libs) -lSDL2_mixer

