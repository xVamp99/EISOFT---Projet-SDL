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
SDL_Surface* thinkImage;
SDL_Surface* clickImage;
SDL_Surface* happyImage;
SDL_Surface* angryImage;
int currentImageState;

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

Mix_Music *gameOverMusic = NULL;


Mix_Chunk *beepSound = NULL;
Mix_Chunk *hoverSound = NULL;
Mix_Chunk *sourisSound = NULL;
Mix_Music *enigmeMusic = NULL;
Mix_Music *winMusic = NULL;
Uint32 lastBeepTime = 0;
Uint32 beepInterval = BEEP_INTERVAL_START;
int beepChannel = -1;
int shouldBeep = 1;
PlayerStats player = {0, 3, 1, 0, 0};
GameState currentState = MENU_STATE;
int lastHoveredButton = -1;

int pointInRect(Point p, SDL_Rect r) {
    return (p.x >= r.x) && (p.x < r.x + r.w) && (p.y >= r.y) && (p.y < r.y + r.h);
}

SDL_Surface* loadImage(const char *path) {
    SDL_Surface *img = IMG_Load(path);
    if (!img) printf("Error loading %s: %s\n", path, IMG_GetError());
    return img;
}

void renderCenteredText(SDL_Surface *screen, TTF_Font *font, const char *text, 
                       SDL_Rect rect, SDL_Color color) {
    SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, color);
    if (textSurface) {
        SDL_Rect dstRect = {
            rect.x + (rect.w - textSurface->w)/2,
            rect.y + (rect.h - textSurface->h)/2,
            textSurface->w,
            textSurface->h
        };
        SDL_BlitSurface(textSurface, NULL, screen, &dstRect);
        SDL_FreeSurface(textSurface);
    }
}

void updateBeepSound(float progress) {
    if (!shouldBeep || progress <= 0) {
        if (beepChannel != -1) {
            Mix_HaltChannel(beepChannel);
            beepChannel = -1;
        }
        return;
    }

    Uint32 currentTime = SDL_GetTicks();
    beepInterval = BEEP_INTERVAL_MIN + 
                  (Uint32)((BEEP_INTERVAL_START - BEEP_INTERVAL_MIN) * progress);
    
    if (currentTime - lastBeepTime > beepInterval) {
        if (beepSound) {
            if (beepChannel != -1) {
                Mix_HaltChannel(beepChannel);
            }
            beepChannel = Mix_PlayChannel(-1, beepSound, 0);
        }
        lastBeepTime = currentTime;
    }
}

void drawTimer(SDL_Surface *screen, float progress) {
    SDL_Rect bgRect = {(WINDOW_WIDTH-TIMER_WIDTH)/2, TIMER_Y, TIMER_WIDTH, TIMER_HEIGHT};
    SDL_FillRect(screen, &bgRect, SDL_MapRGB(screen->format, 100, 100, 100));
    
    int width = (int)(TIMER_WIDTH * progress);
    Uint8 r = progress > 0.5 ? (Uint8)(255 * (1-progress)*2) : 255;
    Uint8 g = progress < 0.5 ? (Uint8)(255 * progress*2) : 255;
    
    SDL_Rect timerRect = {(WINDOW_WIDTH-TIMER_WIDTH)/2, TIMER_Y, width, TIMER_HEIGHT};
    SDL_FillRect(screen, &timerRect, SDL_MapRGB(screen->format, r, g, 0));
}

void renderPlayerStats(SDL_Surface *screen, TTF_Font *font) {
    char statsText[100];
    SDL_Color white = {255, 255, 255, 0};
    SDL_Color gold = {255, 215, 0, 0};

    sprintf(statsText, "Score: %d", player.score);
    renderCenteredText(screen, font, statsText, (SDL_Rect){WINDOW_WIDTH-200, 20, 180, 30}, gold);
    
    sprintf(statsText, "Lives: %d", player.lives);
    renderCenteredText(screen, font, statsText, (SDL_Rect){WINDOW_WIDTH-200, 60, 180, 30}, white);
    
    sprintf(statsText, "Level: %d", player.level);
    renderCenteredText(screen, font, statsText, (SDL_Rect){WINDOW_WIDTH-200, 100, 180, 30}, white);
    
    sprintf(statsText, "Survived: %d/%d", player.questionsSurvived, QUESTIONS_TO_WIN);
    renderCenteredText(screen, font, statsText, (SDL_Rect){WINDOW_WIDTH-200, 140, 180, 30}, white);
}

int loadQuestions(const char *filename, QuizQuestion *questions) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open %s\n", filename);
        return 0;
    }
    
    char line[MAX_LINE_LENGTH];
    int count = 0;
    
    while (count < MAX_QUESTIONS && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        strncpy(questions[count].question, line, MAX_LINE_LENGTH);
        
        for (int i = 0; i < 3; i++) {
            if (!fgets(line, sizeof(line), file)) break;
            line[strcspn(line, "\n")] = 0;
            strncpy(questions[count].answers[i], line, MAX_LINE_LENGTH);
        }
        
        if (fgets(line, sizeof(line), file)) {
            questions[count].correctAnswer = atoi(line);
        }
        
        count++;
    }
    
    fclose(file);
    return count;
}

void showGameOver(SDL_Surface *screen, TTF_Font *font) {
    SDL_Surface *gameOverBg = loadImage("over.png");
    if (!gameOverBg) {
        printf("Failed to load game over background\n");
        return;
    }

    // Stop current music and play game over music
 //   if (enigmeMusic) Mix_HaltMusic();
 //   if (gameOverMusic) Mix_PlayMusic(gameOverMusic, 0);

    SDL_BlitSurface(gameOverBg, NULL, screen, NULL);
    
    char gameOverText[100];
    sprintf(gameOverText, "Final Score: %d", player.score);
    
    SDL_Color red = {255, 0, 0, 0};
    SDL_Color white = {255, 255, 255, 0};
    
    renderCenteredText(screen, font, "Game Over!", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 100, WINDOW_WIDTH, 50}, red);
    renderCenteredText(screen, font, gameOverText, 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 30, WINDOW_WIDTH, 50}, white);
    renderCenteredText(screen, font, "Click to return to menu", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 + 40, WINDOW_WIDTH, 50}, white);
    
    SDL_Flip(screen);
    
    SDL_Event event;
    int waiting = 1;
    while (waiting) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN) {
                waiting = 0;
            }
        }
        SDL_Delay(10);
    }

    // Stop game over music and return to menu music
  //  if (gameOverMusic) Mix_HaltMusic();
  //  if (enigmeMusic) Mix_PlayMusic(enigmeMusic, -1);

    SDL_FreeSurface(gameOverBg);
}

void showWinScreen(SDL_Surface *screen, TTF_Font *font) {
    SDL_Surface *winBg = loadImage("win.png");
    if (!winBg) {
        printf("Failed to load win background\n");
        return;
    }

  //  if (enigmeMusic) Mix_HaltMusic();
   // if (winMusic) Mix_PlayMusic(winMusic, 0);

    SDL_BlitSurface(winBg, NULL, screen, NULL);
    
    char winText[100];
    sprintf(winText, "Final Score: %d", player.score);
    
    SDL_Color gold = {255, 215, 0, 0};
    SDL_Color white = {255, 255, 255, 0};
    
    renderCenteredText(screen, font, "You Win!", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 100, WINDOW_WIDTH, 50}, gold);
    renderCenteredText(screen, font, winText, 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 30, WINDOW_WIDTH, 50}, white);
    renderCenteredText(screen, font, "Click to return to menu", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 + 40, WINDOW_WIDTH, 50}, white);
    
    SDL_Flip(screen);
    
    SDL_Event event;
    int waiting = 1;
    while (waiting) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN) {
                waiting = 0;
            }
        }
        SDL_Delay(10);
    }

  //  if (winMusic) Mix_HaltMusic();
  //  if (enigmeMusic) Mix_PlayMusic(enigmeMusic, -1);

    SDL_FreeSurface(winBg);
}




void runQuizGame(SDL_Surface *screen, TTF_Font *font, QuizQuestion *questions, int questionCount, TTF_Font *font1) {
    // Define colors (only keeping used ones)
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color progressGreen = {0, 200, 0, 255};
    SDL_Color progressBgColor = {50, 50, 50, 255};

    // Load game assets
    SDL_Surface *bg = loadImage("2D.png");
    SDL_Surface *questionButton = loadImage("button.png");
    SDL_Surface *answerButtons[3] = {NULL};
    SDL_Surface *answerButtonsHover[3] = {NULL};
    SDL_Surface *nextButton = loadImage("next.png");
    
    // Load emotion animation frames
    SDL_Surface *thinkImgs[3] = {loadImage("think1.png"), loadImage("think2.png"), loadImage("think3.png")};
    SDL_Surface *clickImgs[3] = {loadImage("click1.png"), loadImage("click2.png"), loadImage("click3.png")};
    SDL_Surface *happyImgs[3] = {loadImage("happy1.png"), loadImage("happy2.png"), loadImage("happy3.png")};
    SDL_Surface *angryImgs[3] = {loadImage("angry1.png"), loadImage("angry2.png"), loadImage("angry3.png")};
    
    int currentEmotion = 0;
    int currentFrame = 0;
    Uint32 lastFrameTime = SDL_GetTicks();
    Uint32 frameDelay = 200;
    
    // Load answer buttons
    answerButtons[0] = loadImage("a.png");
    answerButtons[1] = loadImage("b.png");
    answerButtons[2] = loadImage("c.png");
    answerButtonsHover[0] = loadImage("a1.png");
    answerButtonsHover[1] = loadImage("b1.png");
    answerButtonsHover[2] = loadImage("c1.png");

    // UI positions
    SDL_Rect questionRect = {((WINDOW_WIDTH - BUTTON_WIDTH)/2)-60, 100, BUTTON_WIDTH, BUTTON_HEIGHT};
    SDL_Rect answerRects[3];
    int answerSpacing = 20;
    int totalWidth = 3*BUTTON_WIDTH + 2*answerSpacing;
    int startX = (WINDOW_WIDTH - totalWidth)/2;
    
    for (int i = 0; i < 3; i++) {
        answerRects[i] = (SDL_Rect){
            startX + i*(BUTTON_WIDTH + answerSpacing),
            questionRect.y + questionRect.h + 150,
            BUTTON_WIDTH, 
            BUTTON_HEIGHT
        };
    }

    SDL_Rect nextButtonRect = {
        (WINDOW_WIDTH - NEXT_BUTTON_WIDTH)/2,
        answerRects[0].y + BUTTON_HEIGHT + 40,
        NEXT_BUTTON_WIDTH,
        NEXT_BUTTON_HEIGHT
    };

    // Game state
    srand(time(NULL));
    int currentQuestion = rand() % questionCount;
    int selectedAnswer = -1;
    int showingAnswer = 0;
    int running = 1;
    int showNextButton = 0;
    Uint32 startTime = SDL_GetTicks();
    int lastHoveredAnswer = -1;

    while (running) {
        // Handle frame animation
        if (SDL_GetTicks() - lastFrameTime > frameDelay) {
            currentFrame = (currentFrame + 1) % 3;
            lastFrameTime = SDL_GetTicks();
        }

        // Timer logic
        float timeLeft = TIMER_DURATION - (SDL_GetTicks() - startTime)/1000.0f;
        float progress = timeLeft/TIMER_DURATION;
        
        if (timeLeft <= 0 && !showingAnswer) {
            showingAnswer = 1;
            selectedAnswer = -1;
            shouldBeep = 0;
            player.lives--;
            currentEmotion = 3;
            showNextButton = 1;
            
            if (player.lives <= 0) {
                currentState = GAMEOVER_STATE;
                running = 0;
            }
        }
        
        // Mouse hover detection
        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        int hoveredAnswer = -1;
        
        if (!showingAnswer) {
            currentEmotion = 0;
            for (int i = 0; i < 3; i++) {
                if (pointInRect(mouse, answerRects[i])) {
                    currentEmotion = 1;
                    hoveredAnswer = i;
                    
                    // Play sound only when entering a new button
                    if (hoveredAnswer != lastHoveredAnswer && sourisSound) {
                        Mix_PlayChannel(-1, sourisSound, 0);
                    }
                    break;
                }
            }
            lastHoveredAnswer = hoveredAnswer;
        }
        
        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                currentState = MENU_STATE;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
                currentState = MENU_STATE;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                Point mouse = {event.button.x, event.button.y};
                
                if (!showingAnswer) {
                    for (int i = 0; i < 3; i++) {
                        if (pointInRect(mouse, answerRects[i])) {
                            selectedAnswer = i;
                            showingAnswer = 1;
                            shouldBeep = 0;
                            showNextButton = 1;
                            
                            if (selectedAnswer == questions[currentQuestion].correctAnswer) {
                                currentEmotion = 2;
                                player.score += 100 * player.level;
                                player.questionsSurvived++;
                                if (player.questionsSurvived >= QUESTIONS_TO_WIN) {
                                    currentState = WIN_STATE;
                                    running = 0;
                                }
                            } else {
                                currentEmotion = 3;
                                player.lives--;
                                if (player.lives <= 0) {
                                    currentState = GAMEOVER_STATE;
                                    running = 0;
                                }
                            }
                            break;
                        }
                    }
                }
                else if (showNextButton && pointInRect(mouse, nextButtonRect)) {
                    currentEmotion = 0;
                    currentQuestion = rand() % questionCount;
                    selectedAnswer = -1;
                    showingAnswer = 0;
                    showNextButton = 0;
                    startTime = SDL_GetTicks();
                    shouldBeep = 1;
                }
            }
        }

        // Update beep sound
        updateBeepSound(progress);

        // Render everything
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
        SDL_BlitSurface(bg, NULL, screen, NULL);
        
        if (!showingAnswer) {
            drawTimer(screen, progress);
        }
        
        // Display animated emotion image
        SDL_Rect emotionRect = {20, 20, 100, 100};
        SDL_Surface *currentEmotionImg = NULL;
        
        switch(currentEmotion) {
            case 0: currentEmotionImg = thinkImgs[currentFrame]; break;
            case 1: currentEmotionImg = clickImgs[currentFrame]; break;
            case 2: currentEmotionImg = happyImgs[currentFrame]; break;
            case 3: currentEmotionImg = angryImgs[currentFrame]; break;
        }
        
        if (currentEmotionImg) {
            SDL_BlitSurface(currentEmotionImg, NULL, screen, &emotionRect);
        }

        // Draw question and answers
        SDL_BlitSurface(questionButton, NULL, screen, &questionRect);
        renderCenteredText(screen, font, questions[currentQuestion].question, questionRect, black);
        
        for (int i = 0; i < 3; i++) {
            if (hoveredAnswer == i && !showingAnswer && answerButtonsHover[i]) {
                SDL_BlitSurface(answerButtonsHover[i], NULL, screen, &answerRects[i]);
            } else {
                SDL_BlitSurface(answerButtons[i], NULL, screen, &answerRects[i]);
            }
            
            SDL_Color textColor = black;
            if (showingAnswer) {
                if (i == questions[currentQuestion].correctAnswer) {
                    textColor = green;
                } else if (i == selectedAnswer) {
                    textColor = red;
                }
            }
            renderCenteredText(screen, font, questions[currentQuestion].answers[i], answerRects[i], textColor);
        }

        if (showNextButton && nextButton) {
            SDL_BlitSurface(nextButton, NULL, screen, &nextButtonRect);
            renderCenteredText(screen, font, "Next", nextButtonRect, gold);
        }

        // Render player stats
        char statsText[128];
        SDL_Rect statsRect = {20, 140, 300, 100};
        sprintf(statsText, "Lives: %d\nLevel: %d\nScore: %d", player.lives, player.level, player.score);
        SDL_Surface *statsSurface = TTF_RenderText_Blended(font1, statsText, gold);
        if (statsSurface) {
            SDL_BlitSurface(statsSurface, NULL, screen, &statsRect);
            SDL_FreeSurface(statsSurface);
        }

        /* PROGRESS BAR */
        // Background
        SDL_Rect progressBg = {10, WINDOW_HEIGHT - 40, WINDOW_WIDTH - 20, 20};
        SDL_FillRect(screen, &progressBg, SDL_MapRGB(screen->format, progressBgColor.r, progressBgColor.g, progressBgColor.b));
        
        // Foreground (progress)
        int progressWidth = (int)((WINDOW_WIDTH - 20) * ((float)player.questionsSurvived / QUESTIONS_TO_WIN));
        progressWidth = progressWidth < 0 ? 0 : progressWidth;
        progressWidth = progressWidth > WINDOW_WIDTH - 20 ? WINDOW_WIDTH - 20 : progressWidth;
        
        SDL_Rect progressBar = {10, WINDOW_HEIGHT - 40, progressWidth, 20};
        SDL_FillRect(screen, &progressBar, SDL_MapRGB(screen->format, progressGreen.r, progressGreen.g, progressGreen.b));
        
        // Progress text
        char progressText[50];
        sprintf(progressText, "%d/%d", player.questionsSurvived, QUESTIONS_TO_WIN);
        SDL_Surface* progressTextSurface = TTF_RenderText_Blended(font1, progressText, gold);
        if (progressTextSurface) {
            SDL_Rect textRect = {
                (WINDOW_WIDTH - progressTextSurface->w)/2,
                WINDOW_HEIGHT - 40,
                progressTextSurface->w,
                progressTextSurface->h
            };
            SDL_BlitSurface(progressTextSurface, NULL, screen, &textRect);
            SDL_FreeSurface(progressTextSurface);
        }

        SDL_Flip(screen);
        SDL_Delay(10);
    }

    // Clean up resources
    SDL_FreeSurface(bg);
    SDL_FreeSurface(questionButton);
    for (int i = 0; i < 3; i++) {
        SDL_FreeSurface(answerButtons[i]);
        SDL_FreeSurface(answerButtonsHover[i]);
        SDL_FreeSurface(thinkImgs[i]);
        SDL_FreeSurface(clickImgs[i]);
        SDL_FreeSurface(happyImgs[i]);
        SDL_FreeSurface(angryImgs[i]);
    }
    SDL_FreeSurface(nextButton);
}
int main() {
    SDL_Surface *screen = NULL;
    TTF_Font *font = NULL;
    TTF_Font *font1 = NULL;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_SWSURFACE);
    if (!screen) {
        printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() == -1) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        return 1;
    }
    
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        return 1;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mix_OpenAudio failed: %s\n", Mix_GetError());
        return 1;
    }
    
    Mix_AllocateChannels(16);
    beepSound = Mix_LoadWAV("beep.wav");
    hoverSound = Mix_LoadWAV("hover.wav");
    sourisSound = Mix_LoadWAV("souris.wav");
    enigmeMusic = Mix_LoadMUS("enigme.wav");
    winMusic = Mix_LoadMUS("win.wav");
    gameOverMusic = Mix_LoadMUS("over.wav");
    
    if (!beepSound || !hoverSound || !sourisSound) {
        printf("Warning: Could not load sound effects\n");
    }
    if (!enigmeMusic) {
        printf("Warning: Could not load enigme music\n");
    }
    if (!winMusic) {
        printf("Warning: Could not load win music\n");
    }
    if (!gameOverMusic) {
        printf("Warning: Could not load game over music\n");
    }
    
    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        printf("TTF_OpenFont failed: %s\n", TTF_GetError());
        return 1;
    }
    
    font1 = TTF_OpenFont("zzz.ttf", 24);
    if (!font1) {
        printf("TTF_OpenFont1 failed: %s\n", TTF_GetError());
        return 1;
    }

    QuizQuestion questions[MAX_QUESTIONS];
    int questionCount = loadQuestions("fichier.txt", questions);
    if (questionCount == 0) {
        return 1;
    }

    SDL_Surface *menuBg = loadImage("2D.png");
    SDL_Surface *quizButton = loadImage("quiz.png");
    SDL_Surface *quizButtonHover = loadImage("quiz2.png");
    SDL_Surface *puzzleButton = loadImage("puzzle.png");
    SDL_Surface *puzzleButtonHover = loadImage("puzzle2.png");
    SDL_Surface *otherButton = loadImage("other.png");       // Add your other program button
    SDL_Surface *otherButtonHover = loadImage("other2.png"); // Add hover version
    
    if (!menuBg || !quizButton || !quizButtonHover || !puzzleButton || !puzzleButtonHover) {
        printf("Failed to load menu assets\n");
        return 1;
    }

    // Button positions - arranged in a row with spacing
    int buttonSpacing = 20;
    int totalButtonsWidth = quizButton->w + puzzleButton->w + otherButton->w + 2*buttonSpacing;
    int startX = (screen->w - totalButtonsWidth) / 2;
    
    SDL_Rect quizButtonRect = {
        startX,
        (screen->h / 2) - 100,
        quizButton->w,
        quizButton->h
    };
    
    SDL_Rect puzzleButtonRect = {
        quizButtonRect.x + quizButton->w + buttonSpacing,
        quizButtonRect.y,
        puzzleButton->w,
        puzzleButton->h
    };

    SDL_Rect otherButtonRect = {
        puzzleButtonRect.x + puzzleButton->w + buttonSpacing,
        quizButtonRect.y,
        otherButton->w,
        otherButton->h
    };

    if (enigmeMusic) {
        Mix_PlayMusic(enigmeMusic, -1);
    }

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && currentState == MENU_STATE) {
                Point mouse = {event.button.x, event.button.y};
                if (pointInRect(mouse, quizButtonRect)) {
                    Mix_PlayChannel(-1, hoverSound, 0);
                    if (enigmeMusic) Mix_HaltMusic();
                    currentState = GAME_STATE;
                    player = (PlayerStats){0, 3, 1, 0, 0};
                }
                else if (pointInRect(mouse, puzzleButtonRect)) {
                    Mix_PlayChannel(-1, hoverSound, 0);
                    if (enigmeMusic) Mix_HaltMusic();
                    currentState = PUZZLE_STATE;
                    // Initialize puzzle game state here if needed
                }
                else if (pointInRect(mouse, otherButtonRect)) {
                    Mix_PlayChannel(-1, hoverSound, 0);
                    if (enigmeMusic) Mix_HaltMusic();
                    currentState = OTHER_STATE;
                    // Initialize your other program here
                }
            }
        }

        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        int isHoveringQuiz = pointInRect(mouse, quizButtonRect);
        int isHoveringPuzzle = pointInRect(mouse, puzzleButtonRect);
        int isHoveringOther = pointInRect(mouse, otherButtonRect);

        // Play hover sound when entering a button
        if ((isHoveringQuiz || isHoveringPuzzle || isHoveringOther) && lastHoveredButton == -1) {
            if (sourisSound) Mix_PlayChannel(-1, sourisSound, 0);
        }
        
        // Track which button is being hovered
        if (isHoveringQuiz) {
            lastHoveredButton = 0;
        } else if (isHoveringPuzzle) {
            lastHoveredButton = 1;
        } else if (isHoveringOther) {
            lastHoveredButton = 2;
        } else {
            lastHoveredButton = -1;
        }

        switch(currentState) {
            case MENU_STATE:
                SDL_BlitSurface(menuBg, NULL, screen, NULL);
                SDL_BlitSurface(isHoveringQuiz ? quizButtonHover : quizButton, NULL, screen, &quizButtonRect);
                SDL_BlitSurface(isHoveringPuzzle ? puzzleButtonHover : puzzleButton, NULL, screen, &puzzleButtonRect);
                SDL_BlitSurface(isHoveringOther ? otherButtonHover : otherButton, NULL, screen, &otherButtonRect);
                break;
                
            case GAME_STATE:
                runQuizGame(screen, font, questions, questionCount, font1);
                if (currentState == GAMEOVER_STATE) {
                    showGameOver(screen, font);
                    currentState = MENU_STATE;
                    if (enigmeMusic) Mix_PlayMusic(enigmeMusic, -1);
                }
                else if (currentState == WIN_STATE) {
                    showWinScreen(screen, font);
                    currentState = MENU_STATE;
                }
                break;
                
            case PUZZLE_STATE:
                // Add your puzzle game function call here
                // runPuzzleGame(screen, font);
                
                // Example structure for puzzle game:
                // int puzzleResult = runPuzzleGame();
                // if (puzzleResult == PUZZLE_COMPLETE) {
                //     currentState = MENU_STATE;
                //     if (enigmeMusic) Mix_PlayMusic(enigmeMusic, -1);
                // }
                break;
                
            case OTHER_STATE:
                // Add your other program function call here
                // runOtherProgram(screen, font);
                break;
        }

        SDL_Flip(screen);
        SDL_Delay(10);
    }

    // Clean up resources
    if (beepSound) Mix_FreeChunk(beepSound);
    if (hoverSound) Mix_FreeChunk(hoverSound);
    if (sourisSound) Mix_FreeChunk(sourisSound);
    if (enigmeMusic) Mix_FreeMusic(enigmeMusic);
    if (winMusic) Mix_FreeMusic(winMusic);
    if (gameOverMusic) Mix_FreeMusic(gameOverMusic);
    Mix_CloseAudio();
    
    SDL_FreeSurface(menuBg);
    SDL_FreeSurface(quizButton);
    SDL_FreeSurface(quizButtonHover);
    SDL_FreeSurface(puzzleButton);
    SDL_FreeSurface(puzzleButtonHover);
    SDL_FreeSurface(otherButton);
    SDL_FreeSurface(otherButtonHover);
    
    TTF_CloseFont(font);
    TTF_CloseFont(font1);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
