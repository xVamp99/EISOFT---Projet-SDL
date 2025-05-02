#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>

int check_click(SDL_Rect pos, int w, int h, int x, int y) {
    return (x >= pos.x && x <= pos.x + w && y >= pos.y && y <= pos.y + h);
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *screen = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

    // Chargement des images
    SDL_Surface *blurred = IMG_Load("blurred.bmp"); // Image floutée (~600x300)
    SDL_Surface *choice1 = IMG_Load("choice1.bmp"); // Bonne réponse (~150x150)
    SDL_Surface *choice2 = IMG_Load("choice2.bmp");
    SDL_Surface *choice3 = IMG_Load("choice3.bmp");
    SDL_Surface *success = IMG_Load("success.bmp"); // Image succès (800x600)
    SDL_Surface *failure = IMG_Load("failure.bmp"); // Image échec  (800x600)

    if (!blurred || !choice1 || !choice2 || !choice3 || !success || !failure) {
        printf("Erreur de chargement images : %s\n", SDL_GetError());
        return 1;
    }

    // Positions adaptées aux tailles redimensionnées
    SDL_Rect posBlurred = {100, 30};        // Centré (800-600)/2
    SDL_Rect posChoice1 = {100, 350};       // Choix 1
    SDL_Rect posChoice2 = {325, 350};       // Choix 2
    SDL_Rect posChoice3 = {550, 350};       // Choix 3
    SDL_Rect posButton = {500, 500};        // Bouton "Try Again"

    int running = 1;
    int gameState = 0; // 0 = jeu normal, 1 = succès, 2 = échec
    SDL_Event event;

    while (running) {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255)); // Fond blanc

        if (gameState == 0) {
            // Affiche le puzzle
            SDL_BlitSurface(blurred, NULL, screen, &posBlurred);
            SDL_BlitSurface(choice1, NULL, screen, &posChoice1);
            SDL_BlitSurface(choice2, NULL, screen, &posChoice2);
            SDL_BlitSurface(choice3, NULL, screen, &posChoice3);
        }
        else if (gameState == 1) {
            // Affiche succès
            SDL_BlitSurface(success, NULL, screen, NULL);
        }
        else if (gameState == 2) {
            // Affiche échec
            SDL_BlitSurface(failure, NULL, screen, NULL);
        }

        SDL_Flip(screen);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                if (gameState == 0) {
                    // Clics sur les choix
                    if (check_click(posChoice1, choice1->w, choice1->h, x, y)) {
                        gameState = 1; // ✅ Bonne réponse
                    }
                    else if (check_click(posChoice2, choice2->w, choice2->h, x, y)) {
                        gameState = 2; // ❌ Mauvaise réponse
                    }
                    else if (check_click(posChoice3, choice3->w, choice3->h, x, y)) {
                        gameState = 2; // ❌ Mauvaise réponse
                    }
                }
                else if (gameState == 2) {
                    // Si perdu, vérifier clic sur "Try Again"
                    if (x >= posButton.x && x <= posButton.x + 150 && y >= posButton.y && y <= posButton.y + 50) {
                        gameState = 0; // Rejouer
                    }
                }
            }
        }
    }

    // Libération mémoire
    SDL_FreeSurface(blurred);
    SDL_FreeSurface(choice1);
    SDL_FreeSurface(choice2);
    SDL_FreeSurface(choice3);
    SDL_FreeSurface(success);
    SDL_FreeSurface(failure);

    SDL_Quit();
    return 0;
}

