#ifndef MENU_H
#define MENU_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>

// Fonctions d'initialisation
int init_SDL();
int init_TTF();
int init_audio();

// Gestion de la fenêtre et des images/son
SDL_Surface* create_window(int width, int height);
SDL_Surface* load_image(const char *path);
Mix_Music* load_music(const char* path);

// Libération des ressources
void cleanup(SDL_Surface *image, SDL_Surface *ecran);
void cleanup_music(Mix_Music* music);

// Fonction pour afficher le texte (nom du jeu)
void render_text(SDL_Surface *screen, const char *text, int x, int y, TTF_Font *font, SDL_Color color);
// Fonction pour charger et afficher le logo
void render_logo(SDL_Surface *screen, const char *logo_path, int x, int y, int logo_width, int logo_height);

// Fonction pour afficher le fond d'écran redimensionné
void render_background(SDL_Surface* screen, SDL_Surface* bg_image);

// Structure d'un bouton
typedef struct {
    SDL_Rect rect;           // Position et taille du bouton
    SDL_Surface* normal;     // Surface du bouton normal
    SDL_Surface* hover;      // Surface du bouton au survol
    //(PAS MAINTENANT) void (*action)();        // Fonction à appeler lors du clic
} Button;

// Fonction de création et affichage des boutons
Button create_button(int x, int y, int w, int h, const char* normal_img, const char* hover_img);
// Fonction pour afficher les boutons normaux
void render_button_normal(SDL_Surface* screen, Button* button);  // Corrected declaration
// Fonction pour afficher un bouton avec survol
void render_button(SDL_Surface* screen, Button* button, int mouse_x, int mouse_y);

// (PAS MAINTENAT) int handle_button_click(Button* button, SDL_Event* event);

#endif // MENU_H

