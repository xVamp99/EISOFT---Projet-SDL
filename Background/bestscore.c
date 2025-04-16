// main.c - Integration of Score page and nickname page : Jasser Ouni

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h> // Include SDL_mixer
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SCREEN_WIDTH  1637
#define SCREEN_HEIGHT 920
#define MAX_TEXT_LENGTH 20
#define MAX_SCORES 10 // Number of high scores to store
#define SCORE_FILE "scores.dat"
#define MUSIC_FILE "ogmusic.mp3" // Path to background music

// --- Data Structures ---
typedef struct {
    char name[MAX_TEXT_LENGTH];
    int score; // Add a score to the structure
} Score;

typedef enum {
    NICKNAME_SCREEN,
    BEST_SCORE_SCREEN,
    ENGIME_SCREEN // Added new state
} GameState;

// --- Global Variables ---
SDL_Surface* screen = NULL;
SDL_Surface* background = NULL;
SDL_Surface* inputTextSurface = NULL;

SDL_Surface* submitButton = NULL;
SDL_Surface* submitButtonHover = NULL; // Hover state
SDL_Rect submitButtonRect;

SDL_Surface* backButton = NULL;
SDL_Surface* backButtonHover = NULL; // Hover state
SDL_Rect backButtonRect;

SDL_Surface* ExitButton = NULL;  // For best score screen
SDL_Surface* ExitButtonHover = NULL; // Hover state
SDL_Rect ExitButtonRect;

SDL_Surface* texte = NULL;

TTF_Font* textFont = NULL;
TTF_Font* inputFont = NULL;

char playerName[MAX_TEXT_LENGTH] = ""; // Player's nickname
GameState currentState = NICKNAME_SCREEN;
Score highScores[MAX_SCORES];  // Array to store high scores
int numHighScores = 0; // Number of high scores loaded
Mix_Music* music = NULL; // Background music

bool submitButtonHovered = false;
bool backButtonHovered = false;
bool exitButtonHovered = false;

// --- New Global Variables for Engime Screen ---
SDL_Surface* engimeBackground = NULL; // Background for the Engime screen

// --- Function Prototypes ---
void initSDL();
void cleanupSDL();
void handleNicknameEvents(SDL_Event* event, bool* running, char* inputText, int* cursorPos, bool* typing);
void renderNicknameScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* submitButton, SDL_Surface* backButton, SDL_Surface* inputTextSurface, SDL_Rect inputRect, SDL_Rect textRect, char* inputText);
void handleBestScoreEvents(SDL_Event* event, bool* running);
void renderBestScoreScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* backButton, SDL_Surface* ExitButton, SDL_Rect textRect);
void loadScores();
void saveScores();
void addScore(const char* name, int score);
void playMusic();
void stopMusic();
bool isMouseOver(int x, int y, SDL_Rect rect);

// --- New functions for Engime Screen ---
void handleEngimeEvents(SDL_Event* event, bool* running);
void renderEngimeScreen(SDL_Surface* screen, SDL_Surface* engimeBackground);

// --- Function Implementations ---

void initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
        fprintf(stderr, "Erreur SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "Erreur SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
        fprintf(stderr, "Erreur SDL_mixer: %s\n", Mix_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    if (screen == NULL) {
        fprintf(stderr, "Erreur mode vidÃ©o: %s\n", SDL_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    SDL_WM_SetCaption("Game Title", NULL);

    background = IMG_Load("image1.png");
    submitButton = IMG_Load("Submit.png");
    submitButtonHover = IMG_Load("HoverSubmit.png");
    backButton = IMG_Load("Back.png");
    backButtonHover = IMG_Load("HoverBack.png");
    ExitButton = IMG_Load("Exit.png");
    ExitButtonHover = IMG_Load("HoverExit.png");

    if (!background || !submitButton || !backButton || !ExitButton || !submitButtonHover || !backButtonHover || !ExitButtonHover) {
        fprintf(stderr, "Erreur chargement images: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    textFont = TTF_OpenFont("Lovedays.ttf", 40);
    if (textFont == NULL) {
        fprintf(stderr, "Erreur chargement police pour texte: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
    inputFont = TTF_OpenFont("Aovel.ttf", 30);
    if (inputFont == NULL) {
        fprintf(stderr, "Erreur chargement police pour entrÃ©e: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    // Load music (but don't play it yet)
    music = Mix_LoadMUS(MUSIC_FILE);
    if (music == NULL) {
        fprintf(stderr, "Erreur chargement musique: %s\n", Mix_GetError());
    }

    //Initialize button rectangles
    submitButtonRect = (SDL_Rect){ (SCREEN_WIDTH - submitButton->w) / 2, 200 + 30, submitButton->w, submitButton->h };
    backButtonRect = (SDL_Rect){ 40, SCREEN_HEIGHT - 120, 200, 100 }; // Position for back button
    ExitButtonRect = (SDL_Rect){ 900, 550, 200, 100 }; // Example position

    // Load Engime Screen Background
    engimeBackground = IMG_Load("image1.png"); // Assuming same background, change if needed
    if (!engimeBackground) {
        fprintf(stderr, "Erreur chargement du background Engime: %s\n", IMG_GetError());
        // Handle error appropriately (e.g., exit or set a default)
        exit(EXIT_FAILURE);
    }
}

void cleanupSDL() {
    SDL_FreeSurface(texte);
    SDL_FreeSurface(background);
    SDL_FreeSurface(submitButton);
    SDL_FreeSurface(submitButtonHover);
    SDL_FreeSurface(backButton);
    SDL_FreeSurface(backButtonHover);
    SDL_FreeSurface(ExitButton);
    SDL_FreeSurface(ExitButtonHover);
    SDL_FreeSurface(inputTextSurface);
    // Free the Engime background
    SDL_FreeSurface(engimeBackground);
    TTF_CloseFont(textFont);
    TTF_CloseFont(inputFont);
    if (music) {
        Mix_FreeMusic(music);
    }

    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
}

void handleNicknameEvents(SDL_Event* event, bool* running, char* inputText, int* cursorPos, bool* typing) {
    int mouseX, mouseY;

    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                *running = false;
                break;
            case SDL_KEYDOWN:
                if (event->key.keysym.sym == SDLK_ESCAPE) {
                    *running = false; // Exit on ESC key
                }
                if (*typing) {
                    if (event->key.keysym.sym == SDLK_BACKSPACE && *cursorPos > 0) {
                        inputText[--(*cursorPos)] = '\0';
                    } else {
                        SDLKey key = event->key.keysym.sym;
                        if ((key >= SDLK_a && key <= SDLK_z) || (key >= SDLK_0 && key <= SDLK_9) || key == SDLK_SPACE) {
                            if (*cursorPos < MAX_TEXT_LENGTH - 1) {
                                char newChar = (char)key;
                                if (SDL_GetModState() & KMOD_SHIFT) {
                                    newChar = (char)toupper(newChar);
                                }
                                inputText[(*cursorPos)++] = newChar;
                                inputText[*cursorPos] = '\0';
                            }
                        }
                    }
                }
                // Check for Enter key to submit
                if (event->key.keysym.sym == SDLK_RETURN && *typing) {
                    strncpy(playerName, inputText, MAX_TEXT_LENGTH); // Save the player's name
                    playerName[MAX_TEXT_LENGTH - 1] = '\0'; // Ensure null termination
                    addScore(playerName, 100); // Add score and save
                    currentState = BEST_SCORE_SCREEN; // Switch to the best score screen
                }
                break;
            case SDL_MOUSEMOTION:
                mouseX = event->motion.x;
                mouseY = event->motion.y;
                submitButtonHovered = isMouseOver(mouseX, mouseY, submitButtonRect);
                backButtonHovered = isMouseOver(mouseX, mouseY, backButtonRect);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    int x = event->button.x, y = event->button.y;
                    // Check if the mouse is within the input box
                    if (x >= (SCREEN_WIDTH - 303) / 2 && x <= (SCREEN_WIDTH - 303) / 2 + 303 &&
                        y >= 180 && y <= 180 + 60) {
                        *typing = true;
                    } else {
                        *typing = false;
                    }
                    // Check if the submit button is clicked
                    if (isMouseOver(x, y, submitButtonRect)) {
                        strncpy(playerName, inputText, MAX_TEXT_LENGTH); // Save the player's name
                        playerName[MAX_TEXT_LENGTH - 1] = '\0'; // Ensure null termination
                        addScore(playerName, 100); // Add score and save
                        currentState = BEST_SCORE_SCREEN; // Switch to the best score screen
                    }
                    // Check if the back button is clicked
                    if (isMouseOver(x, y, backButtonRect)) {
                        printf("Main Menu (Integri ya taher)\n");
                    }
                }
                break;
        }
    }
}

void renderNicknameScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* submitButton, SDL_Surface* backButton, SDL_Surface* inputTextSurface, SDL_Rect inputRect, SDL_Rect textRect, char* inputText) {
    SDL_BlitSurface(background, NULL, screen, NULL);
    SDL_BlitSurface(texte, NULL, screen, &textRect);
    SDL_FillRect(screen, &inputRect, SDL_MapRGB(screen->format, 255, 255, 255)); // White input box

    // Blit submit button
    if (submitButtonHovered) {
        SDL_BlitSurface(submitButtonHover, NULL, screen, &submitButtonRect);
    } else {
        SDL_BlitSurface(submitButton, NULL, screen, &submitButtonRect);
    }

    // Blit back button
    if (backButtonHovered) {
        SDL_BlitSurface(backButtonHover, NULL, screen, &backButtonRect);
    } else {
        SDL_BlitSurface(backButton, NULL, screen, &backButtonRect);
    }

    if (inputTextSurface) {
        SDL_Rect inputTextPos = {inputRect.x + 5, inputRect.y + 10};  // Adjust for padding
        SDL_BlitSurface(inputTextSurface, NULL, screen, &inputTextPos);
    }

    SDL_Flip(screen);
}

void handleBestScoreEvents(SDL_Event* event, bool* running) {
    int mouseX, mouseY;

    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                *running = false;
                break;
            case SDL_KEYDOWN:
                if (event->key.keysym.sym == SDLK_e) {
                    currentState = ENGIME_SCREEN; // Switch to Engime screen on 'E' key press
                }
                break;
            case SDL_MOUSEMOTION:
                mouseX = event->motion.x;
                mouseY = event->motion.y;
                backButtonHovered = isMouseOver(mouseX, mouseY, backButtonRect);
                exitButtonHovered = isMouseOver(mouseX, mouseY, ExitButtonRect);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    int x = event->button.x, y = event->button.y;

                    // Example back button logic - adjust coordinates
                    if (isMouseOver(x, y, backButtonRect)) {
                        currentState = NICKNAME_SCREEN; // Go back to nickname screen
                        stopMusic(); // Stop music when leaving best score screen
                    }
                    // Example exit button logic - adjust coordinates
                    if (isMouseOver(x, y, ExitButtonRect)) {
                        *running = false;  // Exit the application
                    }
                }
                break;
        }
    }
}

void renderBestScoreScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* backButton, SDL_Surface* ExitButton, SDL_Rect textRect) {
    SDL_BlitSurface(background, NULL, screen, NULL);
    SDL_BlitSurface(texte, NULL, screen, &textRect);

    // Render the player's name
    SDL_Color playerNameColor = {255, 255, 0, 255}; // Yellow
    SDL_Surface* playerNameSurface = TTF_RenderText_Blended(textFont, playerName, playerNameColor);
    SDL_Rect playerNameRect = { (SCREEN_WIDTH - playerNameSurface->w) / 2, 100, playerNameSurface->w, playerNameSurface->h };
    SDL_BlitSurface(playerNameSurface, NULL, screen, &playerNameRect);
    SDL_FreeSurface(playerNameSurface);

    // Display the scores
    int yOffset = 150;
    for (int i = 0; i < numHighScores; i++) {
        char scoreText[100];
        sprintf(scoreText, "%d. %s: %d", i + 1, highScores[i].name, highScores[i].score);
        SDL_Surface* scoreSurface = TTF_RenderText_Blended(textFont, scoreText, playerNameColor);
        SDL_Rect scoreRect = { (SCREEN_WIDTH - scoreSurface->w) / 2, yOffset, scoreSurface->w, scoreSurface->h };
        SDL_BlitSurface(scoreSurface, NULL, screen, &scoreRect);
        SDL_FreeSurface(scoreSurface);
        yOffset += 40;
    }

    // Render buttons
    if (backButtonHovered) {
        SDL_BlitSurface(backButtonHover, NULL, screen, &backButtonRect);
    } else {
        SDL_BlitSurface(backButton, NULL, screen, &backButtonRect);
    }

    if (exitButtonHovered) {
        SDL_BlitSurface(ExitButtonHover, NULL, screen, &ExitButtonRect);
    } else {
        SDL_BlitSurface(ExitButton, NULL, screen, &ExitButtonRect);
    }

    SDL_Flip(screen);
}

void loadScores() {
    FILE* file = fopen(SCORE_FILE, "rb");
    if (file) {
        numHighScores = fread(highScores, sizeof(Score), MAX_SCORES, file);
        fclose(file);
    } else {
        numHighScores = 0;
    }
}

void saveScores() {
    FILE* file = fopen(SCORE_FILE, "wb");
    if (file) {
        fwrite(highScores, sizeof(Score), numHighScores, file);
        fclose(file);
    }
}

// Function to add a new score to the high scores list
void addScore(const char* name, int score) {
    // Create a new score entry
    Score newScore;
    strncpy(newScore.name, name, MAX_TEXT_LENGTH);
    newScore.name[MAX_TEXT_LENGTH - 1] = '\0'; // Ensure null termination
    newScore.score = score;

    // Find the correct position to insert the new score
    int insertPos = 0;
    while (insertPos < numHighScores && highScores[insertPos].score > score) {
        insertPos++;
    }

    // Shift the existing scores down to make space for the new score
    for (int i = numHighScores; i > insertPos; i--) {
        highScores[i] = highScores[i - 1];
    }

    // Insert the new score
    highScores[insertPos] = newScore;
    if (numHighScores < MAX_SCORES) {
        numHighScores++;
    }

    saveScores();
}

void playMusic() {
    if (music && Mix_PlayingMusic() == 0) {
        if (Mix_PlayMusic(music, -1) == -1) {
            fprintf(stderr, "Error playing music: %s\n", Mix_GetError());
        }
    }
}

void stopMusic() {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
}

bool isMouseOver(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

void handleEngimeEvents(SDL_Event* event, bool* running) {
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                *running = false;
                break;
            case SDL_KEYDOWN:
                if (event->key.keysym.sym == SDLK_ESCAPE) {
                    currentState = BEST_SCORE_SCREEN;
                }
                break;
        }
    }
}

void renderEngimeScreen(SDL_Surface* screen, SDL_Surface* engimeBackground) {
    SDL_BlitSurface(engimeBackground, NULL, screen, NULL);
    SDL_Flip(screen);
}

int main(int argc, char* argv[]) {
    bool running = true;
    SDL_Event event;
    char inputText[MAX_TEXT_LENGTH] = "";
    int cursorPos = 0;
    bool typing = false;
    SDL_Color textColor = {0, 0, 0};

    initSDL();
    loadScores();

    SDL_Rect textRect = {(SCREEN_WIDTH - 200) / 2-150, 100, 200, 50};
    SDL_Rect inputRect = {(SCREEN_WIDTH - 303) / 2, 180, 303, 60};

    SDL_Color textColor2 = {0, 0, 0};
    texte = TTF_RenderText_Solid(textFont, "Enter Your Name/Nickname", textColor2);
    if (!texte) {
        fprintf(stderr, "Error rendering text: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    while (running) {
        switch (currentState) {
            case NICKNAME_SCREEN:
                handleNicknameEvents(&event, &running, inputText, &cursorPos, &typing);

                SDL_FreeSurface(inputTextSurface);
                inputTextSurface = TTF_RenderText_Solid(inputFont, inputText, textColor);

                renderNicknameScreen(screen, background, texte, submitButton, backButton, inputTextSurface, inputRect, textRect, inputText);
                break;
            case BEST_SCORE_SCREEN:
                playMusic();
                handleBestScoreEvents(&event, &running);
                renderBestScoreScreen(screen, background, texte, backButton, ExitButton, textRect);
                break;
            case ENGIME_SCREEN:
                stopMusic();
                handleEngimeEvents(&event, &running);
                renderEngimeScreen(screen, engimeBackground);
                break;
        }
        SDL_Delay(10);
    }

    cleanupSDL();
    return 0;
}

