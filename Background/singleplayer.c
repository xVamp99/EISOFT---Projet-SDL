#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SCREEN_WIDTH  1190
#define SCREEN_HEIGHT 668
#define MAX_TEXT_LENGTH 20
#define MAX_SCORES 10
#define SCORE_FILE "scores.dat"
#define MUSIC_FILE "ogmusic.mp3"
#define SCROLL_SPEED 4
#define CAMERA_SPEED 2

typedef struct {
    char name[MAX_TEXT_LENGTH];
    int score;
} Score;

typedef enum {
    NICKNAME_SCREEN,
    BEST_SCORE_SCREEN,
    GAME_SCREEN
} GameState;

typedef struct {
    SDL_Surface *image;
    SDL_Rect camera;
} Background;

typedef struct {
    SDL_Surface *sprite;
    SDL_Rect pos;
} Player;

SDL_Surface* screen = NULL;
SDL_Surface* background = NULL;
SDL_Surface* inputTextSurface = NULL;
SDL_Surface* submitButton = NULL;
SDL_Surface* submitButtonHover = NULL;
SDL_Rect submitButtonRect;
SDL_Surface* backButton = NULL;
SDL_Surface* backButtonHover = NULL;
SDL_Rect backButtonRect;
SDL_Surface* ExitButton = NULL;
SDL_Surface* ExitButtonHover = NULL;
SDL_Rect ExitButtonRect;
SDL_Surface* texte = NULL;
TTF_Font* textFont = NULL;
TTF_Font* inputFont = NULL;
char playerName[MAX_TEXT_LENGTH] = "";
GameState currentState = NICKNAME_SCREEN;
Score highScores[MAX_SCORES];
int numHighScores = 0;
Mix_Music* music = NULL;
bool submitButtonHovered = false;
bool backButtonHovered = false;
bool exitButtonHovered = false;

Background gameBackground;
Player player;
int keys[SDLK_LAST] = {0};

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

void initGameScreen();
void handleGameEvents(SDL_Event* event, bool* running, Uint32 startTime);
void renderGameScreen(SDL_Surface* screen, Uint32 startTime);
void initBackground(Background *bg, const char *path);
void initPlayer(Player *p, const char *spritePath, int x, int y);
void scrolling(SDL_Rect *camera, Player *p, int bg_width);
void render(SDL_Surface *screen, Background *bg, Player *p);
void handleInput(int *keys, SDL_Event *event);

void initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf Error: %s\n", TTF_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
        fprintf(stderr, "SDL_mixer Error: %s\n", Mix_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    if (screen == NULL) {
        fprintf(stderr, "Video mode error: %s\n", SDL_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    SDL_WM_SetCaption("Reflection", NULL);

    background = IMG_Load("image1.png");
    submitButton = IMG_Load("Submit.png");
    submitButtonHover = IMG_Load("HoverSubmit.png");
    backButton = IMG_Load("Back.png");
    backButtonHover = IMG_Load("HoverBack.png");
    ExitButton = IMG_Load("Exit.png");
    ExitButtonHover = IMG_Load("HoverExit.png");
    if (!background || !submitButton || !backButton || !ExitButton || !submitButtonHover || !backButtonHover || !ExitButtonHover) {
        fprintf(stderr, "Image loading error: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    textFont = TTF_OpenFont("Lovedays.ttf", 40);
    inputFont = TTF_OpenFont("Aovel.ttf", 30);
    if (!textFont || !inputFont) {
        fprintf(stderr, "Font loading error: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    music = Mix_LoadMUS(MUSIC_FILE);
    if (!music) {
        fprintf(stderr, "Music loading error: %s\n", Mix_GetError());
    }

    submitButtonRect = (SDL_Rect){ (SCREEN_WIDTH - submitButton->w) / 2, 200 + 30, submitButton->w, submitButton->h };
    backButtonRect = (SDL_Rect){ 40, SCREEN_HEIGHT - 120, 200, 100 };
    ExitButtonRect = (SDL_Rect){ 900, 550, 200, 100 };

    initGameScreen();
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
    TTF_CloseFont(textFont);
    TTF_CloseFont(inputFont);
    
    if (music) Mix_FreeMusic(music);
    SDL_FreeSurface(gameBackground.image);
    SDL_FreeSurface(player.sprite);

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
                    *running = false;
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
                if (event->key.keysym.sym == SDLK_RETURN && *typing) {
                    strncpy(playerName, inputText, MAX_TEXT_LENGTH);
                    playerName[MAX_TEXT_LENGTH - 1] = '\0';
                    currentState = GAME_SCREEN;
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
                    if (x >= (SCREEN_WIDTH - 303) / 2 && x <= (SCREEN_WIDTH - 303) / 2 + 303 &&
                        y >= 180 && y <= 180 + 60) {
                        *typing = true;
                    } else {
                        *typing = false;
                    }
                    if (isMouseOver(x, y, submitButtonRect)) {
                        strncpy(playerName, inputText, MAX_TEXT_LENGTH);
                        playerName[MAX_TEXT_LENGTH - 1] = '\0';
                        currentState = GAME_SCREEN;
                    }
                    if (isMouseOver(x, y, backButtonRect)) {
                        printf("Main Menu\n");
                    }
                }
                break;
        }
    }
}

void renderNicknameScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* submitButton, SDL_Surface* backButton, SDL_Surface* inputTextSurface, SDL_Rect inputRect, SDL_Rect textRect, char* inputText) {
    SDL_BlitSurface(background, NULL, screen, NULL);
    SDL_BlitSurface(texte, NULL, screen, &textRect);

    SDL_FillRect(screen, &inputRect, SDL_MapRGB(screen->format, 255, 255, 255));

    if (submitButtonHovered) {
        SDL_BlitSurface(submitButtonHover, NULL, screen, &submitButtonRect);
    } else {
        SDL_BlitSurface(submitButton, NULL, screen, &submitButtonRect);
    }

    if (backButtonHovered) {
        SDL_BlitSurface(backButtonHover, NULL, screen, &backButtonRect);
    } else {
        SDL_BlitSurface(backButton, NULL, screen, &backButtonRect);
    }

    if (inputTextSurface) {
        SDL_Rect inputTextPos = {inputRect.x + 5, inputRect.y + 10};
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
            case SDL_MOUSEMOTION:
                mouseX = event->motion.x;
                mouseY = event->motion.y;
                backButtonHovered = isMouseOver(mouseX, mouseY, backButtonRect);
                exitButtonHovered = isMouseOver(mouseX, mouseY, ExitButtonRect);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event->button.button == SDL_BUTTON_LEFT) {
                    int x = event->button.x, y = event->button.y;
                    if (isMouseOver(x, y, backButtonRect)) {
                        currentState = NICKNAME_SCREEN;
                        stopMusic();
                    }
                    if (isMouseOver(x, y, ExitButtonRect)) {
                        *running = false;
                    }
                }
                break;
        }
    }
}

void renderBestScoreScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* backButton, SDL_Surface* ExitButton, SDL_Rect textRect) {
    SDL_BlitSurface(background, NULL, screen, NULL);
    SDL_BlitSurface(texte, NULL, screen, &textRect);

    SDL_Color playerNameColor = {255, 255, 0, 255};
    SDL_Surface* playerNameSurface = TTF_RenderText_Blended(textFont, playerName, playerNameColor);
    SDL_Rect playerNameRect = { (SCREEN_WIDTH - playerNameSurface->w) / 2, 100, playerNameSurface->w, playerNameSurface->h };
    SDL_BlitSurface(playerNameSurface, NULL, screen, &playerNameRect);
    SDL_FreeSurface(playerNameSurface);

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

void addScore(const char* name, int score) {
    Score newScore;
    strncpy(newScore.name, name, MAX_TEXT_LENGTH);
    newScore.name[MAX_TEXT_LENGTH - 1] = '\0';
    newScore.score = score;

    int insertPos = 0;
    while (insertPos < numHighScores && highScores[insertPos].score > score) {
        insertPos++;
    }

    if (numHighScores < MAX_SCORES) numHighScores++;

    for (int i = numHighScores-1; i > insertPos; i--) {
        highScores[i] = highScores[i - 1];
    }

    highScores[insertPos] = newScore;
    saveScores();
}

void playMusic() {
    if (music && !Mix_PlayingMusic()) {
        Mix_PlayMusic(music, -1);
    }
}

void stopMusic() {
    Mix_HaltMusic();
}

bool isMouseOver(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

void initGameScreen() {
    initBackground(&gameBackground, "back1.bmp");
    initPlayer(&player, "player1.png", 50, 380);
    gameBackground.camera.w = SCREEN_WIDTH;
    gameBackground.camera.h = SCREEN_HEIGHT;
    gameBackground.camera.x = 0;
}

void handleGameEvents(SDL_Event* event, bool* running, Uint32 startTime) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT)
            *running = false;
        if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
            *running = false;
        handleInput(keys, event);
    }

    // Player movement
    if (keys[SDLK_RIGHT]) {
        if (player.pos.x + player.pos.w > SCREEN_WIDTH/2 && 
            gameBackground.camera.x < gameBackground.image->w - SCREEN_WIDTH) {
            gameBackground.camera.x += CAMERA_SPEED;
        }
        else if (player.pos.x < SCREEN_WIDTH - player.pos.w) {
            player.pos.x += SCROLL_SPEED;
        }
    }
    if (keys[SDLK_LEFT]) {
        if (player.pos.x < SCREEN_WIDTH/2 && gameBackground.camera.x > 0) {
            gameBackground.camera.x -= CAMERA_SPEED;
        }
        if (player.pos.x > 0) {
            player.pos.x -= SCROLL_SPEED;
        }
    }

    // Win condition
    if (gameBackground.camera.x + SCREEN_WIDTH >= gameBackground.image->w) {
        Uint32 finalTime = (SDL_GetTicks() - startTime) / 1000;
        addScore(playerName, 150 - finalTime);
        currentState = BEST_SCORE_SCREEN;
    }
}

void renderGameScreen(SDL_Surface* screen, Uint32 startTime) {
    render(screen, &gameBackground, &player);

    // Timer rendering
    Uint32 currentTime = SDL_GetTicks() - startTime;
    Uint32 seconds = currentTime / 1000;
    Uint32 minutes = seconds / 60;
    seconds %= 60;

    char timeStr[20];
    sprintf(timeStr, "Time: %02d:%02d", minutes, seconds);
    SDL_Color textColor = {255, 255, 255};
    SDL_Surface* timeSurface = TTF_RenderText_Blended(textFont, timeStr, textColor);
    
    if (timeSurface) {
        SDL_Rect timeRect = {10, 10, timeSurface->w, timeSurface->h};
        SDL_BlitSurface(timeSurface, NULL, screen, &timeRect);
        SDL_FreeSurface(timeSurface);
    }

    SDL_Flip(screen);
}

void initBackground(Background *bg, const char *path) {
    bg->image = SDL_LoadBMP(path);
    if (!bg->image) {
        fprintf(stderr, "Failed to load background image %s: %s\n", path, SDL_GetError());
        exit(EXIT_FAILURE);
    }
    bg->camera.x = 0;
    bg->camera.y = 0;
    bg->camera.w = SCREEN_WIDTH;
    bg->camera.h = SCREEN_HEIGHT;
}

void initPlayer(Player *p, const char *spritePath, int x, int y) {
    p->sprite = IMG_Load(spritePath);
    if (!p->sprite) {
        fprintf(stderr, "Failed to load player sprite %s: %s\n", spritePath, IMG_GetError());
        exit(EXIT_FAILURE);
    }
    p->pos.x = x;
    p->pos.y = y;
    p->pos.w = p->sprite->w;
    p->pos.h = p->sprite->h;
}

void render(SDL_Surface *screen, Background *bg, Player *p) {
    SDL_BlitSurface(bg->image, &bg->camera, screen, NULL);
    SDL_BlitSurface(p->sprite, NULL, screen, &p->pos);
}

void handleInput(int *keys, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        keys[event->key.keysym.sym] = 1;
    } else if (event->type == SDL_KEYUP) {
        keys[event->key.keysym.sym] = 0;
    }
}

int main(int argc, char* argv[]) {
    bool running = true;
    SDL_Event event;
    char inputText[MAX_TEXT_LENGTH] = "";
    int cursorPos = 0;
    bool typing = false;
    SDL_Color textColor = {0, 0, 0, 0};
    SDL_Color textColor2 = {255, 255, 255, 255};
    SDL_Rect inputRect = {(SCREEN_WIDTH - 303) / 2, 180, 303, 60};
    SDL_Rect textRect = {(SCREEN_WIDTH - 600) / 2 + 150, 50, 600, 50};
    SDL_Rect textRect2 = {(SCREEN_WIDTH - 600) / 2 + 200, 50, 600, 50};

    Uint32 gameStartTime = 0;
    GameState previousState = currentState;

    initSDL();
    loadScores();
    playMusic();

    while (running) {
        if (previousState != currentState) {
            if (currentState == GAME_SCREEN) {
                gameStartTime = SDL_GetTicks();
            }
            previousState = currentState;
        }

        switch (currentState) {
            case NICKNAME_SCREEN:
                SDL_FillRect(screen, &screen->clip_rect, SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF));
                SDL_FreeSurface(texte);
                texte = TTF_RenderText_Blended(textFont, "Enter Your Name:", textColor2);
                SDL_BlitSurface(background, NULL, screen, NULL);
                handleNicknameEvents(&event, &running, inputText, &cursorPos, &typing);
                SDL_FreeSurface(inputTextSurface);
                inputTextSurface = TTF_RenderText_Blended(inputFont, inputText, textColor);
                renderNicknameScreen(screen, background, texte, submitButton, backButton, inputTextSurface, inputRect, textRect, inputText);
                break;
            case BEST_SCORE_SCREEN:
                SDL_FillRect(screen, &screen->clip_rect, SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF));
                SDL_FreeSurface(texte);
                texte = TTF_RenderText_Blended(textFont, "High Scores", textColor2);
                handleBestScoreEvents(&event, &running);
                renderBestScoreScreen(screen, background, texte, backButton, ExitButton, textRect2);
                break;
            case GAME_SCREEN:
                handleGameEvents(&event, &running, gameStartTime);
                renderGameScreen(screen, gameStartTime);
                break;
        }
    }

    cleanupSDL();
    return 0;
}
