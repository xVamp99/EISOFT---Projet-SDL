#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL_image.h>

#define SCREEN_WIDTH 1637
#define SCREEN_HEIGHT 920
#define SCROLL_SPEED 10

typedef struct {
    SDL_Surface *image;
    SDL_Rect camera1, camera2;
    SDL_Rect screen_pos1, screen_pos2;
} Background;

typedef struct {
    SDL_Surface *sprite;
    SDL_Rect pos;
} Player;

// Declare the function from bestscore.c
void show_bestscore(int winner);

void initBackground(Background *bg, const char *path) {
    bg->image = SDL_LoadBMP(path);
    if (!bg->image) {
        printf("Failed to load background image: %s\n", SDL_GetError());
        exit(1);
    }

    bg->camera1.x = 0;
    bg->camera1.y = 0;
    bg->camera1.w = SCREEN_WIDTH / 2;
    bg->camera1.h = SCREEN_HEIGHT;

    bg->camera2.x = 0;
    bg->camera2.y = 0;
    bg->camera2.w = SCREEN_WIDTH / 2;
    bg->camera2.h = SCREEN_HEIGHT;

    bg->screen_pos1.x = 0;
    bg->screen_pos1.y = 0;

    bg->screen_pos2.x = SCREEN_WIDTH / 2;
    bg->screen_pos2.y = 0;
}

void initPlayer(Player *p, const char *spritePath, int x, int y) {
    p->sprite = SDL_LoadBMP(spritePath);
    if (!p->sprite) {
        printf("Failed to load player sprite: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetColorKey(p->sprite, SDL_SRCCOLORKEY, SDL_MapRGB(p->sprite->format, 255, 0, 255));
    p->pos.x = x;
    p->pos.y = y;
}

void handleInput(int *keys, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN)
        keys[event->key.keysym.sym] = 1;
    else if (event->type == SDL_KEYUP)
        keys[event->key.keysym.sym] = 0;
}

void scrolling(SDL_Rect *camera, Player *p, int direction, int bg_width, int bg_height) {
    if (direction == 0 && camera->x + camera->w < bg_width)
        camera->x += SCROLL_SPEED;
    else if (direction == 1 && camera->x > 0)
        camera->x -= SCROLL_SPEED;
    else if (direction == 2 && camera->y > 0)
        camera->y -= SCROLL_SPEED;
    else if (direction == 3 && camera->y + camera->h < bg_height)
        camera->y += SCROLL_SPEED;
}

void render(SDL_Surface *screen, Background *bg, Player *p1, Player *p2) {
    SDL_BlitSurface(bg->image, &bg->camera1, screen, &bg->screen_pos1);
    SDL_BlitSurface(bg->image, &bg->camera2, screen, &bg->screen_pos2);

    SDL_Rect drawP1 = { p1->pos.x - bg->camera1.x, p1->pos.y - bg->camera1.y };
    SDL_Rect drawP2 = { p2->pos.x - bg->camera2.x + SCREEN_WIDTH / 2, p2->pos.y - bg->camera2.y };

    SDL_BlitSurface(p1->sprite, NULL, screen, &drawP1);
    SDL_BlitSurface(p2->sprite, NULL, screen, &drawP2);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface *screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    if (!screen) {
        printf("Video mode set failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Background bg;
    initBackground(&bg, "back1.bmp");

    Player p1, p2;
    initPlayer(&p1, "player1.bmp", 50, 300);
    initPlayer(&p2, "player2.bmp", 100, 300);

    int running = 1;
    SDL_Event event;
    int keys[SDLK_LAST] = {0};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
            handleInput(keys, &event);
        }

        // Player 1 movement
        if (keys[SDLK_RIGHT]) {
            if (p1.pos.x > bg.camera1.x + SCREEN_WIDTH / 4)
                scrolling(&bg.camera1, &p1, 0, bg.image->w, bg.image->h);
            else
                p1.pos.x += SCROLL_SPEED;
        }
        if (keys[SDLK_LEFT]) {
            if (p1.pos.x < bg.camera1.x + SCREEN_WIDTH / 4)
                scrolling(&bg.camera1, &p1, 1, bg.image->w, bg.image->h);
            else
                p1.pos.x -= SCROLL_SPEED;
        }

        // Player 2 movement
        if (keys[SDLK_d]) {
            if (p2.pos.x > bg.camera2.x + SCREEN_WIDTH / 4)
                scrolling(&bg.camera2, &p2, 0, bg.image->w, bg.image->h);
            else
                p2.pos.x += SCROLL_SPEED;
        }
        if (keys[SDLK_a]) {
            if (p2.pos.x < bg.camera2.x + SCREEN_WIDTH / 4)
                scrolling(&bg.camera2, &p2, 1, bg.image->w, bg.image->h);
            else
                p2.pos.x -= SCROLL_SPEED;
        }

        // Check if Player 1 reaches the end of the background
        if (p1.pos.x + p1.sprite->w >= bg.image->w) {
            show_bestscore(1); // Player 1 wins
            running = 0;
        }

        // Check if Player 2 reaches the end of the background
        if (p2.pos.x + p2.sprite->w >= bg.image->w) {
            show_bestscore(2); // Player 2 wins
            running = 0;
        }

        render(screen, &bg, &p1, &p2);
        SDL_Flip(screen);
        SDL_Delay(16);
    }

    SDL_FreeSurface(bg.image);
    SDL_FreeSurface(p1.sprite);
    SDL_FreeSurface(p2.sprite);
    SDL_Quit();
    return 0;
}

