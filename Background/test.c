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
#define SCROLL_SPEED 2 // Adjusted scroll speed

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
    SDL_Rect camera1, camera2;
    SDL_Rect screen_pos1, screen_pos2;
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
Player player1, player2;
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
void handleGameEvents(SDL_Event* event, bool* running);
void renderGameScreen(SDL_Surface* screen);
void initBackground(Background *bg, const char *path);
void initPlayer(Player *p, const char *spritePath, int x, int y);
void scrolling(SDL_Rect *camera, Player *p, int direction, int bg_width, int bg_height);
void render(SDL_Surface *screen, Background *bg, Player *p1, Player *p2);
void handleInput(int *keys, SDL_Event *event);

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
    SDL_WM_SetCaption("Reflection", NULL);

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

    music = Mix_LoadMUS(MUSIC_FILE);
    if (music == NULL) {
        fprintf(stderr, "Erreur chargement musique: %s\n", Mix_GetError());
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
    if (music) {
        Mix_FreeMusic(music);
    }

    SDL_FreeSurface(gameBackground.image);
    SDL_FreeSurface(player1.sprite);
    SDL_FreeSurface(player2.sprite);

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

    for (int i = numHighScores; i > insertPos; i--) {
        highScores[i] = highScores[i - 1];
    }

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

void initGameScreen() {
    initBackground(&gameBackground, "back1.bmp");

    // Initialize player 1 on the left side
    initPlayer(&player1, "player1.png", 50, 380);

    // Initialize player 2 on the right side
    initPlayer(&player2, "player2.png", SCREEN_WIDTH / 2 + 50, 380);

    gameBackground.camera1.w = SCREEN_WIDTH / 2;
    gameBackground.camera1.h = SCREEN_HEIGHT;
    gameBackground.camera1.x = 0; // Start at the beginning for player 1

    gameBackground.camera2.w = SCREEN_WIDTH / 2;
    gameBackground.camera2.h = SCREEN_HEIGHT;
    gameBackground.camera2.x = 0; // Also start at the beginning for player 2's camera, it will scroll independently

    gameBackground.screen_pos1.x = 0;
    gameBackground.screen_pos1.y = 0;

    gameBackground.screen_pos2.x = SCREEN_WIDTH / 2;
    gameBackground.screen_pos2.y = 0;
}

void handleGameEvents(SDL_Event* event, bool* running) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT)
            *running = false;
        if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
            *running = false;
        handleInput(keys, event);
    }

    // Player 1 movement
    if (keys[SDLK_RIGHT]) {
        // Scrolling Background
        if (player1.pos.x > SCREEN_WIDTH / 4 && gameBackground.camera1.x < gameBackground.image->w / 2 - gameBackground.camera1.w) {
            scrolling(&gameBackground.camera1, &player1, 0, gameBackground.image->w / 2, gameBackground.image->h);
        }
        // Keep Player1 visible, move only if not at right edge of split screen
        if (player1.pos.x < SCREEN_WIDTH / 2 - player1.sprite->w) {
            player1.pos.x += SCROLL_SPEED;
        }
    }
    if (keys[SDLK_LEFT]) {
        // Scrolling Background
        if (player1.pos.x < SCREEN_WIDTH / 4 && gameBackground.camera1.x > 0) {
            scrolling(&gameBackground.camera1, &player1, 1, gameBackground.image->w / 2, gameBackground.image->h);
        }

        //Keep Player1 visible, move only if not at left edge of screen
        if (player1.pos.x > 0) {
            player1.pos.x -= SCROLL_SPEED;
        }
    }

    // Player 2 movement
    if (keys[SDLK_d]) {
        // Scrolling Background
        if (player2.pos.x > SCREEN_WIDTH / 2 + SCREEN_WIDTH / 4 && gameBackground.camera2.x < gameBackground.image->w / 2 - gameBackground.camera2.w) {
            scrolling(&gameBackground.camera2, &player2, 0, gameBackground.image->w / 2, gameBackground.image->h);
        }
        //Keep Player 2 visible
        if (player2.pos.x < SCREEN_WIDTH - player2.sprite->w) {
            player2.pos.x += SCROLL_SPEED;
        }
    }
    if (keys[SDLK_a]) {
        // Scrolling Background
        if (player2.pos.x < SCREEN_WIDTH / 2 + SCREEN_WIDTH / 4 && gameBackground.camera2.x > 0) {
            scrolling(&gameBackground.camera2, &player2, 1, gameBackground.image->w / 2, gameBackground.image->h);
        }
        //Keep Player 2 visible
        if (player2.pos.x > SCREEN_WIDTH / 2) {
            player2.pos.x -= SCROLL_SPEED;
        }
    }

    // Check win conditions
    if (gameBackground.camera1.x + gameBackground.camera1.w >= gameBackground.image->w / 2 && player1.pos.x + player1.sprite->w >= SCREEN_WIDTH/2) {
        addScore(playerName, 150);
        currentState = BEST_SCORE_SCREEN;
    }

    if (gameBackground.camera2.x + gameBackground.camera2.w >= gameBackground.image->w / 2 && player2.pos.x + player2.sprite->w >= SCREEN_WIDTH) {
        addScore(playerName, 100);
        currentState = BEST_SCORE_SCREEN;
    }
}

void renderGameScreen(SDL_Surface* screen) {
    render(screen, &gameBackground, &player1, &player2);
    SDL_Flip(screen);
}

void initBackground(Background *bg, const char *path) {
    bg->image = SDL_LoadBMP(path);
    if (!bg->image) {
        fprintf(stderr, "Failed to load background image %s: %s\n", path, SDL_GetError());
        exit(EXIT_FAILURE);
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

void scrolling(SDL_Rect *camera, Player *p, int direction, int bg_width, int bg_height) {
    if (direction == 0) { // Right
        camera->x += SCROLL_SPEED;
        if (camera->x + camera->w > bg_width)
            camera->x = bg_width - camera->w;
    } else if (direction == 1) { // Left
        camera->x -= SCROLL_SPEED;
        if (camera->x < 0)
            camera->x = 0;
    }
}

void render(SDL_Surface *screen, Background *bg, Player *p1, Player *p2) {
    SDL_BlitSurface(bg->image, &bg->camera1, screen, &bg->screen_pos1);
    SDL_BlitSurface(p1->sprite, NULL, screen, &p1->pos);

    SDL_BlitSurface(bg->image, &bg->camera2, screen, &bg->screen_pos2);
    SDL_BlitSurface(p2->sprite, NULL, screen, &p2->pos);
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

    initSDL();
    loadScores();
    playMusic();

    while (running) {
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
                handleGameEvents(&event, &running);
                renderGameScreen(screen);
                break;
        }
    }

    cleanupSDL();
    return 0;
}

