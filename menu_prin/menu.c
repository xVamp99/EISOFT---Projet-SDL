#include "menu.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <stdio.h>

int init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("Erreur SDL_Init: %s\n", SDL_GetError());
        return 0;
    }
    return 1;
}

int init_TTF() {
    if (TTF_Init() == -1) {
        printf("Erreur TTF_Init: %s\n", TTF_GetError());
        return 0;
    }
    return 1;
}

int init_audio() {
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
        printf("Erreur Mix_OpenAudio: %s\n", Mix_GetError());
        return 0;
    }
    return 1;
}

SDL_Surface* create_window(int width, int height) {
    SDL_Surface *screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen) {
        printf("Erreur SDL_SetVideoMode: %s\n", SDL_GetError());
        return NULL;
    }
    return screen;
}

SDL_Surface* load_image(const char *path) {
    SDL_Surface *img = IMG_Load(path);
    if (!img) {
        printf("Erreur IMG_Load: %s\n", IMG_GetError());
        return NULL;
    }
    return img;
}

Mix_Music* load_music(const char* path) {
    Mix_Music *music = Mix_LoadMUS(path);
    if (!music) {
        printf("Erreur Mix_LoadMUS: %s\n", Mix_GetError());
    }
    return music;
}

void cleanup(SDL_Surface *image, SDL_Surface *ecran) {
    if (image) {
        SDL_FreeSurface(image);
    }
    if (ecran) {
        SDL_FreeSurface(ecran);
    }
    TTF_Quit();
    SDL_Quit();
}

void cleanup_music(Mix_Music* music) {
    if (music) {
        Mix_FreeMusic(music);
    }
    Mix_CloseAudio();
}

// Fonction pour afficher le texte (nom du jeu)
void render_text(SDL_Surface *screen, const char *text, int x, int y, TTF_Font *font, SDL_Color color) {
    SDL_Surface *message = TTF_RenderText_Solid(font, text, color);
    if (message == NULL) {
        printf("Erreur lors du rendu du texte: %s\n", TTF_GetError());
        return;
    }

    SDL_Rect dest = {x, y};
    SDL_BlitSurface(message, NULL, screen, &dest);
    SDL_FreeSurface(message);
}

// Fonction pour redimensionner l'image 
SDL_Surface* resize_image(SDL_Surface* src, int width, int height) {
    // Créer une nouvelle surface avec la taille désirée
    SDL_Surface* resized = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, src->format->BitsPerPixel,
                                                 src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
    if (resized == NULL) {
        printf("Erreur de création de la surface redimensionnée: %s\n", SDL_GetError());
        return NULL;
    }

    // Redimensionner l'image avec SDL_SoftStretch
    if (SDL_SoftStretch(src, NULL, resized, NULL) != 0) {
        printf("Erreur de redimensionnement: %s\n", SDL_GetError());
        SDL_FreeSurface(resized);
        return NULL;
    }

    return resized;
}

// Fonction pour afficher l'image de fond redimensionnée
void render_background(SDL_Surface* screen, SDL_Surface* bg_image) {
    SDL_Rect bg_rect = {0, 0}; // position de l'image de fond
    SDL_Surface* resized_bg = resize_image(bg_image, screen->w, screen->h); // redimensionner à la taille de l'écran
    if (resized_bg) {
        SDL_BlitSurface(resized_bg, NULL, screen, &bg_rect); // afficher l'image redimensionnée
        SDL_FreeSurface(resized_bg); // libérer la mémoire de l'image redimensionnée
    }
}

// Fonction pour afficher le logo
void render_logo(SDL_Surface *screen, const char *logo_path, int x, int y, int logo_width, int logo_height) {
    // Charger le logo
    SDL_Surface *logo = load_image(logo_path);
    if (!logo) {
        printf("Erreur lors du chargement du logo\n");
        return;
    }

    // Redimensionner l'image du logo
    SDL_Surface *resized_logo = resize_image(logo, logo_width, logo_height);
    if (resized_logo) {
        SDL_Rect pos = {x, y};
        // Afficher l'image redimensionnée
        SDL_BlitSurface(resized_logo, NULL, screen, &pos);
        SDL_FreeSurface(resized_logo);  // Libérer la mémoire
    }
    SDL_FreeSurface(logo);  // Libérer la surface d'origine
}

// Fonction pour créer un bouton
Button create_button(int x, int y, int w, int h, const char* normal_img, const char* hover_img)
{
    Button button;
    button.rect.x = x;
    button.rect.y = y;
    button.rect.w = w;
    button.rect.h = h;
    button.normal = load_image(normal_img);
    button.hover = load_image(hover_img);
    return button;
}

// Fonction pour afficher un bouton avec survol

void render_button(SDL_Surface* screen, Button* button, int mouse_x, int mouse_y) {
    // Si la souris est sur le bouton, afficher l'image hover
    if (mouse_x >= button->rect.x && mouse_x <= button->rect.x + button->rect.w &&
        mouse_y >= button->rect.y && mouse_y <= button->rect.y + button->rect.h) {
        SDL_BlitSurface(button->hover, NULL, screen, &button->rect);
    } else {
        // Sinon afficher l'image normale
        SDL_BlitSurface(button->normal, NULL, screen, &button->rect);
    }
}


