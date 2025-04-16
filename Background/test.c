#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SCREEN_WIDTH  1637
#define SCREEN_HEIGHT 920
#define MAX_TEXT_LENGTH 20
#define MAX_SCORES 10
#define SCORE_FILE "scores.dat"
#define MUSIC_FILE "ogmusic.mp3"
#define SCROLL_SPEED 10

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
    initPlayer(&player1, "player1.bmp", 50, 300);
    initPlayer(&player2, "player2.bmp", 100, 300);
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
        if (player1.pos.x > gameBackground.camera1.x + SCREEN_WIDTH / 4)
            scrolling(&gameBackground.camera1, &player1, 0, gameBackground.image->w, gameBackground.image->h);
        else
            player1.pos.x += SCROLL_SPEED;
    }
    if (keys[SDLK_LEFT]) {
        if (player1.pos.x < gameBackground.camera1.x + SCREEN_WIDTH / 4)
            scrolling(&gameBackground.camera1, &player1, 1, gameBackground.image->w, gameBackground.image->h);
        else
            player1.pos.x -= SCROLL_SPEED;
    }

    // Player 2 movement
    if (keys[SDLK_d]) {
        if (player2.pos.x > gameBackground.camera2.x + SCREEN_WIDTH / 4)
            scrolling(&gameBackground.camera2, &player2, 0, gameBackground.image->w, gameBackground.image->h);
        else
            player2.pos.x += SCROLL_SPEED;
    }
    if (keys[SDLK_a]) {
        if (player2.pos.x < gameBackground.camera2.x + SCREEN_WIDTH / 4)
            scrolling(&gameBackground.camera2, &player2, 1, gameBackground.image->w, gameBackground.image->h);
        else
            player2.pos.x -= SCROLL_SPEED;
    }

    // Check win conditions
    if (player1.pos.x + player1.sprite->w >= gameBackground.image->w) {
        addScore(playerName, 150);
        currentState = BEST_SCORE_SCREEN;
    }
    if (player2.pos.x + player2.sprite->w >= gameBackground.image->w) {
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
        printf("Failed to load background image: %s\n", SDL_GetError());
        exit(1);
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
    p->sprite = SDL_LoadBMP(spritePath);
    if (!p->sprite) {
        printf("Failed to load player sprite: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetColorKey(p->sprite, SDL_SRCCOLORKEY, SDL_MapRGB(p->sprite->format, 255, 0, 255));
    p->pos.x = x;
    p->pos.y = y;
}

void handleInput(int *keys, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN)
        keys[event->key.keysym.sym] = 1;
    else if (event->type == SDL_KEYUP)
        keys[event->key.keysym.sym] = 0;
}

void scrolling(SDL_Rect *camera, Player *p, int direction, int bg_width, int bg_height) {
    if (direction == 0 && camera->x + camera->w < bg_width)
        camera->x += SCROLL_SPEED;
    else if (direction == 1 && camera->x > 0)
        camera->x -= SCROLL_SPEED;
    else if (direction == 2 && camera->y > 0)
        camera->y -= SCROLL_SPEED;
    else if (direction == 3 && camera->y + camera->h < bg_height)
        camera->y += SCROLL_SPEED;
}

void render(SDL_Surface *screen, Background *bg, Player *p1, Player *p2) {
    SDL_BlitSurface(bg->image, &bg->camera1, screen, &bg->screen_pos1);
    SDL_BlitSurface(bg->image, &bg->camera2, screen, &bg->screen_pos2);

    SDL_Rect drawP1 = { p1->pos.x - bg->camera1.x, p1->pos.y - bg->camera1.y };
    SDL_Rect drawP2 = { p2->pos.x - bg->camera2.x + SCREEN_WIDTH / 2, p2->pos.y - bg->camera2.y };

    SDL_BlitSurface(p1->sprite, NULL, screen, &drawP1);
    SDL_BlitSurface(p2->sprite, NULL, screen, &drawP2);
}

int main(int argc, char* argv[]) {
    initSDL();
    loadScores();
    playMusic();

    SDL_Event event;
    bool running = true;
    char inputText[MAX_TEXT_LENGTH] = "";
    int cursorPos = 0;
    bool typing = false;

    SDL_Color textColor = {0, 0, 0};
    SDL_Rect textRect = { (SCREEN_WIDTH - 303) / 2, 100, 303, 50 };
    SDL_Rect inputRect = { (SCREEN_WIDTH - 303) / 2, 180, 303, 60 };

    texte = TTF_RenderText_Solid(textFont, "Enter Your Name:", textColor);

    while (running) {
        switch (currentState) {
            case NICKNAME_SCREEN:
                SDL_FreeSurface(inputTextSurface);
                inputTextSurface = TTF_RenderText_Solid(inputFont, inputText, textColor);
                handleNicknameEvents(&event, &running, inputText, &cursorPos, &typing);
                renderNicknameScreen(screen, background, texte, submitButton, backButton, inputTextSurface, inputRect, textRect, inputText);
                break;
            case BEST_SCORE_SCREEN:
                handleBestScoreEvents(&event, &running);
                renderBestScoreScreen(screen, background, texte, backButton, ExitButton, textRect);
                break;
            case GAME_SCREEN:
                handleGameEvents(&event, &running);
                renderGameScreen(screen);
                break;
        }
        SDL_Delay(16);
    }

    cleanupSDL();
    return 0;
}

