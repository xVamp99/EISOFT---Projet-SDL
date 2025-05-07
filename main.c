#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include "player.h"

// Variables globales pour la police
TTF_Font *font = NULL;
SDL_Color textColor = {255, 255, 255}; // Blanc
SDL_Surface *background = NULL;

void initFont() {
    if(TTF_Init() == -1) {
        fprintf(stderr, "Erreur d'initialisation TTF: %s\n", TTF_GetError());
        exit(1);
    }
    
    font = TTF_OpenFont("arial.ttf", 24); // Chemin vers votre police
    if(!font) {
        fprintf(stderr, "Erreur chargement police: %s\n", TTF_GetError());
        exit(1);
    }
}

void drawText(SDL_Surface *screen, const char *text, int x, int y) {
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    if(!textSurface) {
        fprintf(stderr, "Erreur rendu texte: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Rect textRect = {x, y, 0, 0};
    SDL_BlitSurface(textSurface, NULL, screen, &textRect);
    SDL_FreeSurface(textSurface);
}

int loadBackground() {
    background = IMG_Load("images/background1993.png");
    if(!background) {
        fprintf(stderr, "Erreur chargement background: %s\n", IMG_GetError());
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    SDL_Surface *screen = NULL;
    Player player1, player2;

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Erreur init SDL: %s\n", SDL_GetError());
        return 1;
    }

    screen = SDL_SetVideoMode(1192, 651, 32, SDL_SWSURFACE);
    if(!screen) {
        fprintf(stderr, "Erreur création fenêtre: %s\n", SDL_GetError());
        return 1;
    }

    SDL_WM_SetCaption("2D Game", NULL);
    initFont(); // Initialiser la police
    
    // Charger le background
    if(!loadBackground()) {
        return 1;
    }

    initPlayer(&player1, 50, 380, "images/imageoftheplayer1.png");
    initPlayer(&player2, 200, 380, "images/imageoftheplayer2.png");

    int running = 1;
    SDL_Event event;

    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = 0;
            }
            handleInput(&player1, &event);
            handleInput2(&player2, &event);
        }

        // Afficher le background d'abord
        SDL_BlitSurface(background, NULL, screen, NULL);
        
        drawPlayer(&player1, screen);
        drawPlayer(&player2, screen);
        
        // Afficher les scores et vies
        char scoreText[100];
        sprintf(scoreText, "Joueur - Score: %d Vies: %d", player1.score, player1.lives);
        drawText(screen, scoreText, 10, 10);
        SDL_Flip(screen);
        SDL_Delay(10);
    }

    SDL_FreeSurface(player1.image);
    SDL_FreeSurface(player2.image);
    SDL_FreeSurface(background); // Libérer le background
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
