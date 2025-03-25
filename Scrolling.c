#include <SDL/SDL.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct {
    SDL_Surface *image;
    SDL_Rect camera_pos; // Camera position
} Background;

void initBackground(Background *bg, char *imagePath) {
    bg->image = SDL_LoadBMP(imagePath);
    if (!bg->image) {
        printf("Failed to load background image: %s\n", SDL_GetError());
        exit(1);
    }

    bg->camera_pos.x = 0;
    bg->camera_pos.y = 0;
    bg->camera_pos.w = SCREEN_WIDTH;
    bg->camera_pos.h = SCREEN_HEIGHT;
}

void scrolling(Background *bg, int direction, int dx, int dy) {
    if (direction == 0) { // Scroll right
        bg->camera_pos.x += dx;
    } else if (direction == 1) { // Scroll left
        bg->camera_pos.x -= dx;
    } else if (direction == 2) { // Scroll up
        bg->camera_pos.y -= dy;
    } else if (direction == 3) { // Scroll down
        bg->camera_pos.y += dy;
    }
}

void renderBackground(SDL_Surface *screen, Background *bg) {
    SDL_BlitSurface(bg->image, &bg->camera_pos, screen, NULL);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    Background bg;
    initBackground(&bg, "background.bmp");

    int running = 1, direction = -1, dx = 5, dy = 5;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: direction = 0; break;
                    case SDLK_LEFT: direction = 1; break;
                    case SDLK_UP: direction = 2; break;
                    case SDLK_DOWN: direction = 3; break;
                    case SDLK_ESCAPE: running = 0; break;
                }
            }
        }
        scrolling(&bg, direction, dx, dy);
        renderBackground(screen, &bg);
        SDL_Flip(screen);
        SDL_Delay(16);
    }

    SDL_FreeSurface(bg.image);
    SDL_Quit();
    return 0;
}
