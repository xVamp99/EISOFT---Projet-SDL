#ifndef HEADERS_H
#define HEADERS_H

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define WINDOW_WIDTH 1297
#define WINDOW_HEIGHT 749
#define MAX_QUESTIONS 100
#define MAX_LINE_LENGTH 256
#define BUTTON_WIDTH 400
#define BUTTON_HEIGHT 80
#define BUTTON_SPACING 20
#define QUESTION_Y 150
#define ANSWERS_Y 300
#define TIMER_HEIGHT 20
#define TIMER_WIDTH 500
#define TIMER_Y 80
#define TIMER_DURATION 30
#define BEEP_INTERVAL_START 2000
#define BEEP_INTERVAL_MIN 200
#define NEXT_BUTTON_WIDTH 200
#define NEXT_BUTTON_HEIGHT 60
#define QUESTIONS_TO_WIN 3

typedef enum {
    MENU_STATE,
    GAME_STATE,
    GAMEOVER_STATE,
    WIN_STATE
} GameState;

typedef struct {
    int x, y;
} Point;

typedef struct {
    char question[MAX_LINE_LENGTH];
    char answers[3][MAX_LINE_LENGTH];
    int correctAnswer;
} QuizQuestion;

typedef struct {
    int score;
    int lives;
    int level;
    int consecutiveCorrect;
    int questionsSurvived;
} PlayerStats;

extern Mix_Chunk *beepSound;
extern Mix_Chunk *hoverSound;
extern Mix_Chunk *sourisSound;
extern Mix_Music *enigmeMusic;
extern Mix_Music *winMusic;
extern Uint32 lastBeepTime;
extern Uint32 beepInterval;
extern int beepChannel;
extern int shouldBeep;
extern PlayerStats player;
extern GameState currentState;
extern int lastHoveredButton;

int pointInRect(Point p, SDL_Rect r);
SDL_Surface* loadImage(const char *path);
void renderCenteredText(SDL_Surface *screen, TTF_Font *font, const char *text, SDL_Rect rect, SDL_Color color);
void updateBeepSound(float progress);
void drawTimer(SDL_Surface *screen, float progress);
void renderPlayerStats(SDL_Surface *screen, TTF_Font *font);
int loadQuestions(const char *filename, QuizQuestion *questions);
void showGameOver(SDL_Surface *screen, TTF_Font *font);
void showWinScreen(SDL_Surface *screen, TTF_Font *font);
void runQuizGame(SDL_Surface *screen, TTF_Font *font, QuizQuestion *questions, int questionCount);

#endif
