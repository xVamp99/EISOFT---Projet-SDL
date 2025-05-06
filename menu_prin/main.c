#include "menu.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Initialisation SDL, TTF et Audio
    if (!init_SDL() || !init_TTF() || !init_audio()) {
        return 1;
    }

    // Création de la fenêtre
    SDL_Surface *ecran = create_window(1860,864);
    if (!ecran) {
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Chargement du background
    SDL_Surface *image = load_image("/home/txmk0307/Documents/MYgame/image1/test.png");
    if (!image) {
        cleanup(NULL, ecran);
        return 1;
    }

    // Chargement de la musique
    Mix_Music *music = load_music("/home/txmk0307/Documents/MYgame/son/lobby.mp3");
    if (music) {
        Mix_PlayMusic(music, -1); // -1 = boucle infinie
    }

    // Charger la police TTF
    TTF_Font *font = TTF_OpenFont("/home/txmk0307/Documents/MYgame/TOUGHWALL.ttf", 55);
    if (!font) {
        printf("Erreur lors du chargement de la police: %s\n", TTF_GetError());
        cleanup(image, ecran);
        return 1;
    }

    SDL_Color color = {0, 137, 123}; // bleu marin pour le texte

    // Position du texte (nom du jeu) en haut à droite
    int text_x = ecran->w - 250; // Décalage à droite
    int text_y = 20; // Décalage du haut

    // Position du logo sous le texte
    int logo_x = ecran->w - 160; // Décalage à droite
    int logo_y = text_y + 60; // Juste en dessous du texte
    
    // Création des boutons
    Button play_button = create_button(50, 0, 600, 400, "/home/txmk0307/Documents/MYgame/image1/boutton/play69.png", "/home/txmk0307/Documents/MYgame/image1/boutton/play_hover11.png");
    Button options_button = create_button(50, 200, 600, 400, "/home/txmk0307/Documents/MYgame/image1/boutton/option.png", "/home/txmk0307/Documents/MYgame/image1/boutton/option_hover.png");
    Button score_button = create_button(50, 400, 600, 400, "/home/txmk0307/Documents/MYgame/image1/boutton/score.png", "/home/txmk0307/Documents/MYgame/image1/boutton/score_hover.png");
    Button story_button = create_button(50, 600, 600, 400, "/home/txmk0307/Documents/MYgame/image1/boutton/histoire.png", "/home/txmk0307/Documents/MYgame/image1/boutton/histoire_hover.png");
    Button quit_button = create_button(1300, 600, 600, 400, "/home/txmk0307/Documents/MYgame/image1/boutton/quitter69.png", "/home/txmk0307/Documents/MYgame/image1/boutton/quitter_hover.png");
    
    // Variables pour la gestion de la souris
    int mouse_x, mouse_y;
    SDL_Event event;
    
    // Boucle principale
    int running = 1;
    while (running) {
        // Gérer les événements
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: // Fermer la fenêtre
                running = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (event.button.x >= quit_button.rect.x && event.button.x <= quit_button.rect.x + quit_button.rect.w &&
                        event.button.y >= quit_button.rect.y && event.button.y <= quit_button.rect.y + quit_button.rect.h) {
                        running = 0;  // Quitter le menu
                    }
                }
                break;
        }
       }

        // Récupérer les positions de la souris
        SDL_GetMouseState(&mouse_x, &mouse_y);
        
        // Affichage du background redimensionné
        render_background(ecran, image);

        // Affichage du texte
        render_text(ecran, "EXPLORA!!", text_x, text_y, font, color);

        // Affichage du logo
        render_logo(ecran, "/home/txmk0307/Documents/MYgame/image1/logo.png", logo_x, logo_y, 100, 100);

        // Affichage des boutons
        render_button(ecran, &play_button, mouse_x, mouse_y);
        render_button(ecran, &options_button, mouse_x, mouse_y);
        render_button(ecran, &score_button, mouse_x, mouse_y);
        render_button(ecran, &story_button, mouse_x, mouse_y);
        render_button(ecran, &quit_button, mouse_x, mouse_y);
        
        // Rafraîchir l'écran
        SDL_Flip(ecran);
        SDL_Delay(16); // Limiter le FPS à 60
    }

    // Nettoyage
    cleanup_music(music);
    cleanup(image, ecran);
    TTF_CloseFont(font);
    return 0;
}

