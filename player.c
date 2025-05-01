#include "player.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>

void initPlayer(Player *player, int x, int y, const char *imagePath) {
    player->image = IMG_Load(imagePath); 
    if (player->image == NULL) {
        fprintf(stderr, "Unable to load image: %s\n", IMG_GetError());
        return;
    }
    player->position.x = x;
    player->position.y = y;
    player->position.w = player->image->w / 18.6; // Largeur d'une frame
    player->position.h = player->image->h / 1; // Hauteur d'une frame
    player->lives = 3;
    player->score = 0;
    player->frame = 0;
}

void drawPlayer(Player *player, SDL_Surface *screen) {
    SDL_Rect srcRect;
    srcRect.x = player->frame * player->position.w;
    srcRect.y = 0;
    srcRect.w = player->position.w;
    srcRect.h = player->position.h;

    SDL_BlitSurface(player->image, &srcRect, screen, &player->position);
}

void handleInput(Player *player, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_LEFT:
                player->position.x -= 5;
                player->frame = (player->frame +1) % 3 ;// Animer le sprite
                player->score=player->score-1;
                break;
            case SDLK_RIGHT:
                player->position.x += 5;
                player->frame = (player->frame +1) % 3 ; // Animer le sprite
                player->score=player->score+1;
                break;
            case SDLK_UP:
                player->position.y -= 5;
                player->frame = (player->frame == 5) ? 6 : 5;
                break;
            case SDLK_DOWN:
                player->position.y += 5;
                player->frame = (player->frame == 5) ? 6 : 5;
                break;
            case SDLK_q:
                player->position.x += 15;
                player->frame = (player->frame == 4) ? 8 : 4;
                player->position.x += 10;
                player->position.y -= 10;
                player->position.y += 10;
                player->position.x += 10;
                player->score=player->score+4;
                break;
            case SDLK_w:
                player->frame = (player->frame == 3) ? 7 : 3;
                break;
            case SDLK_r:
                player->position.x += 20;
                player->frame = (player->frame +1) % 3 ;// Animer le sprite
                player->score=player->score+4;
                break;
                default:
                break;
        }
    }
}
void handleInput2(Player *player, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_y:
                player->position.x -= 5;
                player->frame = (player->frame +1) % 3 ;// Animer le sprite
                player->score=player->score-1;
                break;
            case SDLK_x:
                player->position.x += 5;
                player->frame = (player->frame +1) % 3 ; // Animer le sprite
                player->score=player->score+1;
                break;
            case SDLK_c:
                player->position.y -= 5;
                player->frame = (player->frame == 5) ? 6 : 5;
                break;
            case SDLK_v:
                player->position.y += 5;
                player->frame = (player->frame == 5) ? 6 : 5;
                break;
            case SDLK_b:
                player->position.x += 15;
                player->frame = (player->frame == 4) ? 8 : 4;
                player->position.x += 10;
                player->position.y -= 10;
                player->position.y += 10;
                player->position.x += 10;
                player->score=player->score+4;
                break;
            case SDLK_n:
                player->frame = (player->frame == 3) ? 7 : 3;
                break;
            case SDLK_p:
                player->position.x += 20;
                player->frame = (player->frame +1) % 3 ;// Animer le sprite
                player->score=player->score+4;
                break;
                default:
                break;
        }
    }
}