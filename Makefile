CC = gcc
CFLAGS = -Wall -g `sdl-config --cflags` `pkg-config --cflags SDL_image SDL_ttf`
LDFLAGS = `sdl-config --libs` `pkg-config --libs SDL_image SDL_ttf`

SRC = main.c player.c
OBJ = $(SRC:.c=.o)
EXEC = jeux

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: clean all
