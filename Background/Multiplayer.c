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
#define SCROLL_SPEED 2

typedef struct {
    char winner[MAX_TEXT_LENGTH];
    char opponent[MAX_TEXT_LENGTH];
    int time;
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
SDL_Surface* inputTextSurface1 = NULL;
SDL_Surface* inputTextSurface2 = NULL;
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
char player1Name[MAX_TEXT_LENGTH] = "";
char player2Name[MAX_TEXT_LENGTH] = "";
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
void handleNicknameEvents(SDL_Event* event, bool* running, char* inputText1, int* cursorPos1, char* inputText2, int* cursorPos2, int* activeInput, bool* typing);
void renderNicknameScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* submitButton, SDL_Surface* backButton, 
                         SDL_Surface* inputTextSurface1, SDL_Rect inputRect1, SDL_Surface* inputTextSurface2, SDL_Rect inputRect2, SDL_Rect textRect, char* inputText1, char* inputText2);
void handleBestScoreEvents(SDL_Event* event, bool* running);
void renderBestScoreScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* backButton, SDL_Surface* ExitButton, SDL_Rect textRect);
void loadScores();
void saveScores();
void addScore(const char* winner, const char* opponent, int time);
void playMusic();
void stopMusic();
bool isMouseOver(int x, int y, SDL_Rect rect);

void initGameScreen();
void handleGameEvents(SDL_Event* event, bool* running, Uint32 startTime);
void renderGameScreen(SDL_Surface* screen, Uint32 startTime);
void initBackground(Background *bg, const char *path);
void initPlayer(Player *p, const char *spritePath, int x, int y);
void scrolling(SDL_Rect *camera, Player *p, int direction, int bg_width, int bg_height);
void render(SDL_Surface *screen, Background *bg, Player *p1, Player *p2);
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
        fprintf(stderr, "Image load error: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    textFont = TTF_OpenFont("Lovedays.ttf", 40);
    if (textFont == NULL) {
        fprintf(stderr, "Font error: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
    inputFont = TTF_OpenFont("Aovel.ttf", 30);
    if (inputFont == NULL) {
        fprintf(stderr, "Font error: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    music = Mix_LoadMUS(MUSIC_FILE);
    if (music == NULL) {
        fprintf(stderr, "Music error: %s\n", Mix_GetError());
    }

    submitButtonRect = (SDL_Rect){ (SCREEN_WIDTH - submitButton->w) / 2, 350, submitButton->w, submitButton->h };
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
    SDL_FreeSurface(inputTextSurface1);
    SDL_FreeSurface(inputTextSurface2);
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

void handleNicknameEvents(SDL_Event* event, bool* running, char* inputText1, int* cursorPos1, char* inputText2, int* cursorPos2, int* activeInput, bool* typing) {
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
                    if (event->key.keysym.sym == SDLK_BACKSPACE) {
                        if (*activeInput == 0 && *cursorPos1 > 0) {
                            inputText1[--(*cursorPos1)] = '\0';
                        } else if (*activeInput == 1 && *cursorPos2 > 0) {
                            inputText2[--(*cursorPos2)] = '\0';
                        }
                    } else {
                        SDLKey key = event->key.keysym.sym;
                        if ((key >= SDLK_a && key <= SDLK_z) || (key >= SDLK_0 && key <= SDLK_9) || key == SDLK_SPACE) {
                            if (*activeInput == 0 && *cursorPos1 < MAX_TEXT_LENGTH - 1) {
                                char newChar = (char)key;
                                if (SDL_GetModState() & KMOD_SHIFT) {
                                    newChar = (char)toupper(newChar);
                                }
                                inputText1[(*cursorPos1)++] = newChar;
                                inputText1[*cursorPos1] = '\0';
                            } else if (*activeInput == 1 && *cursorPos2 < MAX_TEXT_LENGTH - 1) {
                                char newChar = (char)key;
                                if (SDL_GetModState() & KMOD_SHIFT) {
                                    newChar = (char)toupper(newChar);
                                }
                                inputText2[(*cursorPos2)++] = newChar;
                                inputText2[*cursorPos2] = '\0';
                            }
                        }
                    }
                }
                if (event->key.keysym.sym == SDLK_RETURN && *typing) {
                    strncpy(player1Name, inputText1, MAX_TEXT_LENGTH);
                    strncpy(player2Name, inputText2, MAX_TEXT_LENGTH);
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
                    SDL_Rect inputRect1 = {(SCREEN_WIDTH - 303) / 2, 180, 303, 60};
                    SDL_Rect inputRect2 = {(SCREEN_WIDTH - 303) / 2, 260, 303, 60};
                    
                    if (isMouseOver(x, y, inputRect1)) {
                        *activeInput = 0;
                        *typing = true;
                    } else if (isMouseOver(x, y, inputRect2)) {
                        *activeInput = 1;
                        *typing = true;
                    } else {
                        *typing = false;
                    }
                    
                    if (isMouseOver(x, y, submitButtonRect)) {
                        if (strlen(inputText1) > 0 && strlen(inputText2) > 0) {
                            strncpy(player1Name, inputText1, MAX_TEXT_LENGTH);
                            strncpy(player2Name, inputText2, MAX_TEXT_LENGTH);
                            currentState = GAME_SCREEN;
                        }
                    }
                    if (isMouseOver(x, y, backButtonRect)) {
                        printf("Main Menu\n");
                    }
                }
                break;
        }
    }
}

void renderNicknameScreen(SDL_Surface* screen, SDL_Surface* background, SDL_Surface* texte, SDL_Surface* submitButton, SDL_Surface* backButton, 
                         SDL_Surface* inputTextSurface1, SDL_Rect inputRect1, SDL_Surface* inputTextSurface2, SDL_Rect inputRect2, SDL_Rect textRect, char* inputText1, char* inputText2) {
    SDL_BlitSurface(background, NULL, screen, NULL);
    SDL_BlitSurface(texte, NULL, screen, &textRect);

    SDL_FillRect(screen, &inputRect1, SDL_MapRGB(screen->format, 255, 255, 255));
    SDL_FillRect(screen, &inputRect2, SDL_MapRGB(screen->format, 255, 255, 255));

    SDL_Color labelColor = {255, 255, 255, 255};
    SDL_Surface* label1 = TTF_RenderText_Blended(textFont, "Player 1:", labelColor);
    SDL_Surface* label2 = TTF_RenderText_Blended(textFont, "Player 2:", labelColor);
    SDL_Rect label1Pos = {inputRect1.x - 200, inputRect1.y + 10, label1->w, label1->h};
    SDL_Rect label2Pos = {inputRect2.x - 200, inputRect2.y + 10, label2->w, label2->h};
    SDL_BlitSurface(label1, NULL, screen, &label1Pos);
    SDL_BlitSurface(label2, NULL, screen, &label2Pos);
    SDL_FreeSurface(label1);
    SDL_FreeSurface(label2);

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

    if (inputTextSurface1) {
        SDL_Rect inputTextPos1 = {inputRect1.x + 5, inputRect1.y + 10};
        SDL_BlitSurface(inputTextSurface1, NULL, screen, &inputTextPos1);
    }
    if (inputTextSurface2) {
        SDL_Rect inputTextPos2 = {inputRect2.x + 5, inputRect2.y + 10};
        SDL_BlitSurface(inputTextSurface2, NULL, screen, &inputTextPos2);
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

    SDL_Color scoreColor = {255, 255, 0, 255};
    int yOffset = 150;
    for (int i = 0; i < numHighScores; i++) {
        char scoreText[100];
        sprintf(scoreText, "%d. Winner: %s (vs %s) - Time: %d sec", 
               i + 1, highScores[i].winner, highScores[i].opponent, highScores[i].time);
        SDL_Surface* scoreSurface = TTF_RenderText_Blended(textFont, scoreText, scoreColor);
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

void addScore(const char* winner, const char* opponent, int time) {
    Score newScore;
    strncpy(newScore.winner, winner, MAX_TEXT_LENGTH);
    newScore.winner[MAX_TEXT_LENGTH - 1] = '\0';
    strncpy(newScore.opponent, opponent, MAX_TEXT_LENGTH);
    newScore.opponent[MAX_TEXT_LENGTH - 1] = '\0';
    newScore.time = time;

    int insertPos = 0;
    while (insertPos < numHighScores && highScores[insertPos].time < newScore.time) {
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
    initPlayer(&player1, "player1.png", 50, 380);
    initPlayer(&player2, "player2.png", SCREEN_WIDTH / 2 + 50, 380);

    gameBackground.camera1.w = SCREEN_WIDTH / 2;
    gameBackground.camera1.h = SCREEN_HEIGHT;
    gameBackground.camera1.x = 0;

    gameBackground.camera2.w = SCREEN_WIDTH / 2;
    gameBackground.camera2.h = SCREEN_HEIGHT;
    gameBackground.camera2.x = 0;

    gameBackground.screen_pos1.x = 0;
    gameBackground.screen_pos1.y = 0;

    gameBackground.screen_pos2.x = SCREEN_WIDTH / 2;
    gameBackground.screen_pos2.y = 0;
}

void handleGameEvents(SDL_Event* event, bool* running, Uint32 startTime) {
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT)
            *running = false;
        if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
            *running = false;
        handleInput(keys, event);
    }

    if (keys[SDLK_RIGHT]) {
        if (player1.pos.x > SCREEN_WIDTH / 4 && gameBackground.camera1.x < gameBackground.image->w / 2 - gameBackground.camera1.w) {
            scrolling(&gameBackground.camera1, &player1, 0, gameBackground.image->w / 2, gameBackground.image->h);
        }
        if (player1.pos.x < SCREEN_WIDTH / 2 - player1.sprite->w) {
            player1.pos.x += SCROLL_SPEED;
        }
    }
    if (keys[SDLK_LEFT]) {
        if (player1.pos.x < SCREEN_WIDTH / 4 && gameBackground.camera1.x > 0) {
            scrolling(&gameBackground.camera1, &player1, 1, gameBackground.image->w / 2, gameBackground.image->h);
        }
        if (player1.pos.x > 0) {
            player1.pos.x -= SCROLL_SPEED;
        }
    }

    if (keys[SDLK_d]) {
        if (player2.pos.x > SCREEN_WIDTH / 2 + SCREEN_WIDTH / 4 && gameBackground.camera2.x < gameBackground.image->w / 2 - gameBackground.camera2.w) {
            scrolling(&gameBackground.camera2, &player2, 0, gameBackground.image->w / 2, gameBackground.image->h);
        }
        if (player2.pos.x < SCREEN_WIDTH - player2.sprite->w) {
            player2.pos.x += SCROLL_SPEED;
        }
    }
    if (keys[SDLK_a]) {
        if (player2.pos.x < SCREEN_WIDTH / 2 + SCREEN_WIDTH / 4 && gameBackground.camera2.x > 0) {
            scrolling(&gameBackground.camera2, &player2, 1, gameBackground.image->w / 2, gameBackground.image->h);
        }
        if (player2.pos.x > SCREEN_WIDTH / 2) {
            player2.pos.x -= SCROLL_SPEED;
        }
    }

    if (gameBackground.camera1.x + gameBackground.camera1.w >= gameBackground.image->w / 2 && 
        player1.pos.x + player1.sprite->w >= SCREEN_WIDTH/2) {
        Uint32 finalTime = (SDL_GetTicks() - startTime) / 1000;
        addScore(player1Name, player2Name, finalTime);
        currentState = BEST_SCORE_SCREEN;
    }

    if (gameBackground.camera2.x + gameBackground.camera2.w >= gameBackground.image->w / 2 && 
        player2.pos.x + player2.sprite->w >= SCREEN_WIDTH) {
        Uint32 finalTime = (SDL_GetTicks() - startTime) / 1000;
        addScore(player2Name, player1Name, finalTime);
        currentState = BEST_SCORE_SCREEN;
    }
}

void renderGameScreen(SDL_Surface* screen, Uint32 startTime) {
    render(screen, &gameBackground, &player1, &player2);

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
        fprintf(stderr, "Background error: %s\n", SDL_GetError());
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
        fprintf(stderr, "Player sprite error: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }
    p->pos.x = x;
    p->pos.y = y;
    p->pos.w = p->sprite->w;
    p->pos.h = p->sprite->h;
}

void scrolling(SDL_Rect *camera, Player *p, int direction, int bg_width, int bg_height) {
    if (direction == 0) {
        camera->x += SCROLL_SPEED;
        if (camera->x + camera->w > bg_width)
            camera->x = bg_width - camera->w;
    } else if (direction == 1) {
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
    char inputText1[MAX_TEXT_LENGTH] = "";
    char inputText2[MAX_TEXT_LENGTH] = "";
    int cursorPos1 = 0, cursorPos2 = 0;
    int activeInput = 0;
    bool typing = false;
    SDL_Color textColor = {0, 0, 0, 0};
    SDL_Color textColor2 = {255, 255, 255, 255};
    SDL_Rect inputRect1 = {(SCREEN_WIDTH - 303) / 2, 180, 303, 60};
    SDL_Rect inputRect2 = {(SCREEN_WIDTH - 303) / 2, 260, 303, 60};
    SDL_Rect textRect = {(SCREEN_WIDTH - 600) / 2 + 150, 50, 600, 50};
    SDL_Rect textRect2 = {(SCREEN_WIDTH - 600) / 2 + 200, 50, 600, 50};

    Uint32 gameStartTime = 0;
    GameState previousState = currentState;

    initSDL();
    loadScores();

    while (running) {
        GameState oldState = previousState;
        if (previousState != currentState) {
            if (oldState == BEST_SCORE_SCREEN) {
                stopMusic();
            }

            if (currentState == BEST_SCORE_SCREEN) {
                playMusic();
            }
            if (currentState == GAME_SCREEN) {
                gameStartTime = SDL_GetTicks();
            }

            previousState = currentState;
        }

        switch (currentState) {
            case NICKNAME_SCREEN:
                SDL_FillRect(screen, &screen->clip_rect, SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF));
                SDL_FreeSurface(texte);
                texte = TTF_RenderText_Blended(textFont, "Enter Players Names:", textColor2);
                handleNicknameEvents(&event, &running, inputText1, &cursorPos1, inputText2, &cursorPos2, &activeInput, &typing);
                SDL_FreeSurface(inputTextSurface1);
                SDL_FreeSurface(inputTextSurface2);
                inputTextSurface1 = TTF_RenderText_Blended(inputFont, inputText1, textColor);
                inputTextSurface2 = TTF_RenderText_Blended(inputFont, inputText2, textColor);
                renderNicknameScreen(screen, background, texte, submitButton, backButton, 
                                    inputTextSurface1, inputRect1, inputTextSurface2, inputRect2, textRect, inputText1, inputText2);
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
