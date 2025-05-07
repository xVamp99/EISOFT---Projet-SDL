#ifndef PLAYER_H
#define PLAYER_H

#include <SDL/SDL.h>

typedef struct {
    SDL_Surface *image;
    SDL_Rect position;
    int lives;
    int score;
    int frame;
} Player;

void initPlayer(Player *player, int x, int y, const char *imagePath);
void drawPlayer(Player *player, SDL_Surface *screen);
void handleInput(Player *player, SDL_Event *event);
void handleInput2(Player *player, SDL_Event *event);

#endif
