#include <SDL/SDL.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct {
    SDL_Surface *image;
    SDL_Rect camera1, camera2;
} Background;

typedef struct {
    SDL_Surface *sprite;
    SDL_Rect pos;
} Player;

void initBackground(Background *bg, char *imagePath) {
    bg->image = SDL_LoadBMP(imagePath);
    if (!bg->image) {
        printf("Failed to load background image: %s\n", SDL_GetError());
        exit(1);
    }

    bg->camera1.x = 0;
    bg->camera1.y = 0;
    bg->camera1.w = SCREEN_WIDTH / 2;
    bg->camera1.h = SCREEN_HEIGHT;

    bg->camera2.x = SCREEN_WIDTH / 2;
    bg->camera2.y = 0;
    bg->camera2.w = SCREEN_WIDTH / 2;
    bg->camera2.h = SCREEN_HEIGHT;
}

void initPlayer(Player *p, char *spritePath, int x, int y) {
    p->sprite = SDL_LoadBMP(spritePath);
    if (!p->sprite) {
        printf("Failed to load player sprite: %s\n", SDL_GetError());
        exit(1);
    }
    p->pos.x = x;
    p->pos.y = y;
}

void render(SDL_Surface *screen, Background *bg, Player *p1, Player *p2) {
    SDL_BlitSurface(bg->image, &bg->camera1, screen, NULL);
    SDL_BlitSurface(p1->sprite, NULL, screen, &p1->pos);
    
    SDL_Rect screenPos2 = { SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT };
    SDL_BlitSurface(bg->image, &bg->camera2, screen, &screenPos2);
    SDL_BlitSurface(p2->sprite, NULL, screen, &p2->pos);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);

    Background bg;
    initBackground(&bg, "background.bmp");

    Player p1, p2;
    initPlayer(&p1, "player1.bmp", 50, SCREEN_HEIGHT / 2);
    initPlayer(&p2, "player2.bmp", SCREEN_WIDTH / 2 + 50, SCREEN_HEIGHT / 2);

    int running = 1;
    SDL_Event event;
    int keys[SDLK_LAST] = {0};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                keys[event.key.keysym.sym] = 1;
            } else if (event.type == SDL_KEYUP) {
                keys[event.key.keysym.sym] = 0;
            }
        }

        if (keys[SDLK_RIGHT]) p1.pos.x += 5;
        if (keys[SDLK_LEFT]) p1.pos.x -= 5;
        if (keys[SDLK_UP]) p1.pos.y -= 5;
        if (keys[SDLK_DOWN]) p1.pos.y += 5;

        if (keys[SDLK_d]) p2.pos.x += 5;
        if (keys[SDLK_a]) p2.pos.x -= 5;
        if (keys[SDLK_w]) p2.pos.y -= 5;
        if (keys[SDLK_s]) p2.pos.y += 5;

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
