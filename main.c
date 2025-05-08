#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Screen dimensions
#define WINDOW_WIDTH 1297
#define WINDOW_HEIGHT 749
#define HELP_WIDTH 670
#define HELP_HEIGHT 500

// Game constants
#define MAX_QUESTIONS 100
#define MAX_LINE_LENGTH 256
#define BUTTON_WIDTH 400
#define BUTTON_HEIGHT 80
#define BUTTON_SPACING 20
#define TIMER_HEIGHT 20
#define TIMER_WIDTH 500
#define TIMER_DURATION 30
#define NEXT_BUTTON_WIDTH 200
#define NEXT_BUTTON_HEIGHT 60
#define QUESTIONS_TO_WIN 3
#define PUZZLE_ANSWER_SPACING 50

// Structure definitions
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

// Game states
typedef enum {
    MENU_STATE,
    GAME_STATE,
    GAMEOVER_STATE,
    WIN_STATE,
    PUZZLE_STATE,
    HELP_STATE,
    PUZZLE_HELP_STATE
} GameState;

// Help screen states
typedef enum {
    HELP_OFF,
    HELP_SCREEN1,
    HELP_SCREEN2,
    HELP_SCREEN3,
    HELP_SCREEN4,
    HELP_SCREEN5,
    HELP_SCREEN6
} HelpState;

// Main game data structure
typedef struct {
    SDL_Surface *screen;
    TTF_Font *font;
    TTF_Font *fontLarge;
    
    Mix_Chunk *beepSound;
    Mix_Chunk *hoverSound;
    Mix_Chunk *clickSound;
    Mix_Music *bgMusic;
    Mix_Music *winMusic;
    Mix_Music *gameOverMusic;
    
    SDL_Surface *menuBg;
    SDL_Surface *gameBg;
    SDL_Surface *quizButton;
    SDL_Surface *quizButtonHover;
    SDL_Surface *puzzleButton;
    SDL_Surface *puzzleButtonHover;
    SDL_Surface *questionButton;
    SDL_Surface *answerButtons[3];
    SDL_Surface *answerButtonsHover[3];
    SDL_Surface *nextButton;
    SDL_Surface *emotionImgs[4][3];
    
    // Puzzle game surfaces
    SDL_Surface *puzzleBg;
    SDL_Surface *answers[3];
    SDL_Surface *successImg;
    SDL_Surface *failureImg;
    
    // Help system surfaces
    SDL_Surface *help_btn;
    SDL_Surface *next_btn;
    SDL_Surface *prev_btn;
    SDL_Surface *exit_btn;
    SDL_Surface *help1_img;
    SDL_Surface *help2_img;
    SDL_Surface *help3_img;
    SDL_Surface *help4_img;
    SDL_Surface *help5_img;
    SDL_Surface *help6_img;
    SDL_Surface *overlay;
    
    QuizQuestion questions[MAX_QUESTIONS];
    int questionCount;
    PlayerStats player;
    GameState currentState;
    HelpState help_state;
    
    int lastHoveredButton;
    int isHoverSoundPlaying;
} GameData;

// Helper functions
int pointInRect(Point p, SDL_Rect r) {
    return (p.x >= r.x) && (p.x < r.x + r.w) && (p.y >= r.y) && (p.y < r.y + r.h);
}

SDL_Surface* loadImageWithAlpha(const char *path) {
    SDL_Surface *loaded = IMG_Load(path);
    if (!loaded) {
        printf("Error loading %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    
    SDL_Surface *optimized = SDL_DisplayFormatAlpha(loaded);
    SDL_FreeSurface(loaded);
    return optimized;
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

// Initialization function
int initGame(GameData *game) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError()); 
        return 0;
    }
    
    game->screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_SWSURFACE);
    if (!game->screen) {
        printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return 0;
    }
    
    if (TTF_Init() == -1) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        return 0;
    }
    
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        return 0;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mix_OpenAudio failed: %s\n", Mix_GetError());
        return 0;
    }
    
    Mix_AllocateChannels(16);
    
    // Load fonts
    game->font = TTF_OpenFont("arial.ttf", 24);
    game->fontLarge = TTF_OpenFont("zzz.ttf", 24);
    if (!game->font || !game->fontLarge) {
        printf("TTF_OpenFont failed: %s\n", TTF_GetError());
        return 0;
    }
    
    // Load sounds
    game->beepSound = Mix_LoadWAV("beep.wav");
    game->hoverSound = Mix_LoadWAV("hover.wav");
    game->clickSound = Mix_LoadWAV("click.wav");
    game->bgMusic = Mix_LoadMUS("enigme.wav");
    game->winMusic = Mix_LoadMUS("win.wav");
    game->gameOverMusic = Mix_LoadMUS("over.wav");
    
    // Load quiz game images
    game->menuBg = loadImageWithAlpha("menu_bg.png");
    game->gameBg = loadImageWithAlpha("game_bg.png");
    game->quizButton = loadImageWithAlpha("quiz.png");
    game->quizButtonHover = loadImageWithAlpha("quiz_hover.png");
    game->puzzleButton = loadImageWithAlpha("puzzle.png");
    game->puzzleButtonHover = loadImageWithAlpha("puzzle_hover.png");
    game->questionButton = loadImageWithAlpha("question_btn.png");
    
    game->answerButtons[0] = loadImageWithAlpha("a.png");
    game->answerButtons[1] = loadImageWithAlpha("b.png");
    game->answerButtons[2] = loadImageWithAlpha("c.png");
    game->answerButtonsHover[0] = loadImageWithAlpha("a_hover.png");
    game->answerButtonsHover[1] = loadImageWithAlpha("b_hover.png");
    game->answerButtonsHover[2] = loadImageWithAlpha("c_hover.png");
    
    game->nextButton = loadImageWithAlpha("next.png");
    
    // Load emotion animations
    const char *emotionFiles[4][3] = {
        {"think1.png", "think2.png", "think3.png"},
        {"click1.png", "click2.png", "click3.png"},
        {"happy1.png", "happy2.png", "happy3.png"},
        {"angry1.png", "angry2.png", "angry3.png"}
    };
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            game->emotionImgs[i][j] = loadImageWithAlpha(emotionFiles[i][j]);
        }
    }
    
    // Load puzzle game images
    game->puzzleBg = loadImageWithAlpha("puzzle_3choices.png");
    game->answers[0] = loadImageWithAlpha("answers1.png");
    game->answers[1] = loadImageWithAlpha("answers2.png");
    game->answers[2] = loadImageWithAlpha("answers3.png");
    game->successImg = SDL_LoadBMP("succes.bmp");
    game->failureImg = SDL_LoadBMP("echec.bmp");
    
    // Load help system images
    game->help_btn = loadImageWithAlpha("help_btn.png");
    game->next_btn = loadImageWithAlpha("next_btn.png");
    game->prev_btn = loadImageWithAlpha("prev_btn.png");
    game->exit_btn = loadImageWithAlpha("exit_btn.png");
    game->help1_img = loadImageWithAlpha("help1.png");
    game->help2_img = loadImageWithAlpha("help2.png");
    game->help3_img = loadImageWithAlpha("help3.png");
    game->help4_img = loadImageWithAlpha("help4.png");
    game->help5_img = loadImageWithAlpha("help5.png");
    game->help6_img = loadImageWithAlpha("help6.png");
    
    // Create overlay for help system
    game->overlay = SDL_CreateRGBSurface(SDL_SWSURFACE, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 
                                       0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(game->overlay, NULL, SDL_MapRGBA(game->overlay->format, 0, 0, 0, 180));
    
    // Load questions
    game->questionCount = loadQuestions("questions.txt", game->questions);
    if (game->questionCount == 0) {
        return 0;
    }
    
    // Initialize player stats
    game->player.score = 0;
    game->player.lives = 3;
    game->player.level = 1;
    game->player.consecutiveCorrect = 0;
    game->player.questionsSurvived = 0;
    
    game->currentState = MENU_STATE;
    game->help_state = HELP_OFF;
    game->lastHoveredButton = -1;
    game->isHoverSoundPlaying = 0;
    
    SDL_WM_SetCaption("Quiz Game with Help System", NULL);
    return 1;
}

// Cleanup function
void cleanupGame(GameData *game) {
    // Free surfaces
    SDL_FreeSurface(game->menuBg);
    SDL_FreeSurface(game->gameBg);
    SDL_FreeSurface(game->quizButton);
    SDL_FreeSurface(game->quizButtonHover);
    SDL_FreeSurface(game->puzzleButton);
    SDL_FreeSurface(game->puzzleButtonHover);
    SDL_FreeSurface(game->questionButton);
    
    for (int i = 0; i < 3; i++) {
        SDL_FreeSurface(game->answerButtons[i]);
        SDL_FreeSurface(game->answerButtonsHover[i]);
    }
    
    SDL_FreeSurface(game->nextButton);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            SDL_FreeSurface(game->emotionImgs[i][j]);
        }
    }
    
    // Free puzzle resources
    SDL_FreeSurface(game->puzzleBg);
    for (int i = 0; i < 3; i++) {
        SDL_FreeSurface(game->answers[i]);
    }
    SDL_FreeSurface(game->successImg);
    SDL_FreeSurface(game->failureImg);
    
    // Free help system resources
    SDL_FreeSurface(game->help_btn);
    SDL_FreeSurface(game->next_btn);
    SDL_FreeSurface(game->prev_btn);
    SDL_FreeSurface(game->exit_btn);
    SDL_FreeSurface(game->help1_img);
    SDL_FreeSurface(game->help2_img);
    SDL_FreeSurface(game->help3_img);
    SDL_FreeSurface(game->help4_img);
    SDL_FreeSurface(game->help5_img);
    SDL_FreeSurface(game->help6_img);
    SDL_FreeSurface(game->overlay);
    
    // Free audio
    Mix_FreeChunk(game->beepSound);
    Mix_FreeChunk(game->hoverSound);
    Mix_FreeChunk(game->clickSound);
    Mix_FreeMusic(game->bgMusic);
    Mix_FreeMusic(game->winMusic);
    Mix_FreeMusic(game->gameOverMusic);
    
    // Free fonts
    TTF_CloseFont(game->font);
    TTF_CloseFont(game->fontLarge);
    
    // Quit subsystems
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// Game screens
void drawTimer(GameData *game, float progress) {
    SDL_Rect bgRect = {(WINDOW_WIDTH-TIMER_WIDTH)/2, 80, TIMER_WIDTH, TIMER_HEIGHT};
    SDL_FillRect(game->screen, &bgRect, SDL_MapRGB(game->screen->format, 100, 100, 100));
    
    int width = (int)(TIMER_WIDTH * progress);
    Uint8 r = progress > 0.5 ? (Uint8)(255 * (1-progress)*2) : 255;
    Uint8 g = progress < 0.5 ? (Uint8)(255 * progress*2) : 255;
    
    SDL_Rect timerRect = {(WINDOW_WIDTH-TIMER_WIDTH)/2, 80, width, TIMER_HEIGHT};
    SDL_FillRect(game->screen, &timerRect, SDL_MapRGB(game->screen->format, r, g, 0));
}

void showGameOver(GameData *game) {
    SDL_Surface *loseBg = loadImageWithAlpha("lose.png");
    if (!loseBg) {
        printf("Failed to load lose background\n");
        return;
    }

    SDL_BlitSurface(loseBg, NULL, game->screen, NULL);
    
    char gameOverText[100];
    sprintf(gameOverText, "Final Score: %d", game->player.score);
    
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    
    renderCenteredText(game->screen, game->fontLarge, "Game Over!", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 100, WINDOW_WIDTH, 50}, red);
    renderCenteredText(game->screen, game->font, gameOverText, 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 30, WINDOW_WIDTH, 50}, white);
    renderCenteredText(game->screen, game->font, "Click to return to menu", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 + 40, WINDOW_WIDTH, 50}, white);
    
    SDL_Flip(game->screen);
    
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

    SDL_FreeSurface(loseBg);
}

void showWinScreen(GameData *game) {
    SDL_Surface *winBg = loadImageWithAlpha("win.png");
    if (!winBg) {
        printf("Failed to load win background\n");
        return;
    }

    SDL_BlitSurface(winBg, NULL, game->screen, NULL);
    
    char winText[100];
    sprintf(winText, "Final Score: %d", game->player.score);
    
    SDL_Color gold = {255, 215, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    
    renderCenteredText(game->screen, game->fontLarge, "You Win!", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 100, WINDOW_WIDTH, 50}, gold);
    renderCenteredText(game->screen, game->font, winText, 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 - 30, WINDOW_WIDTH, 50}, white);
    renderCenteredText(game->screen, game->font, "Click to return to menu", 
                     (SDL_Rect){0, WINDOW_HEIGHT/2 + 40, WINDOW_WIDTH, 50}, white);
    
    SDL_Flip(game->screen);
    
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

    SDL_FreeSurface(winBg);
}

// Game modes
void runPuzzleGame(GameData *game) {
    Mix_HaltChannel(-1);
    
    int puzzleRunning = 1;
    int showResult = 0;
    Uint32 resultStartTime = 0;
    int result = 0;
    Uint32 startTime = SDL_GetTicks();

    // Randomly choose between puzzle set 1 or 2
    int puzzleSet = (rand() % 2) + 1; // Will be 1 or 2
    
    // Load the appropriate puzzle background and answers based on the chosen set
    char bgPath[50], answer1Path[50], answer2Path[50], answer3Path[50];
    
    if (puzzleSet == 1) {
        strcpy(bgPath, "puzzle_3choices.png");
        strcpy(answer1Path, "answers1.png");
        strcpy(answer2Path, "answers2.png");
        strcpy(answer3Path, "answers3.png");
    } else {
        strcpy(bgPath, "puzzle_3choices1.png");
        strcpy(answer1Path, "answers4.png");
        strcpy(answer2Path, "answers5.png");
        strcpy(answer3Path, "answers6.png");
    }
    
    // Load the images for this puzzle set
    SDL_Surface *puzzleBg = loadImageWithAlpha(bgPath);
    SDL_Surface *answers[3];
    answers[0] = loadImageWithAlpha(answer1Path);
    answers[1] = loadImageWithAlpha(answer2Path);
    answers[2] = loadImageWithAlpha(answer3Path);

    // Calculate positions with proper spacing
    int answerWidth = 200;
    int answerHeight = 150;
    int totalWidth = (3 * answerWidth) + (2 * PUZZLE_ANSWER_SPACING);
    int startX = (WINDOW_WIDTH - totalWidth) / 2;
    
    SDL_Rect answerRects[3] = {
        {startX-200, 500, answerWidth, answerHeight},
        {startX + answerWidth + PUZZLE_ANSWER_SPACING-100, 500, answerWidth, answerHeight},
        {startX + 2*(answerWidth + PUZZLE_ANSWER_SPACING), 500, answerWidth, answerHeight}
    };

    // Help button position (top right corner)
    SDL_Rect helpButtonRect = {
        WINDOW_WIDTH - game->help_btn->w - 20, 
        20, 
        game->help_btn->w, 
        game->help_btn->h
    };

    while (puzzleRunning) {
        float timeLeft = TIMER_DURATION - (SDL_GetTicks() - startTime)/1000.0f;
        float progress = timeLeft/TIMER_DURATION;
        
        if (timeLeft <= 0 && !showResult) {
            result = 0;
            showResult = 1;
            resultStartTime = SDL_GetTicks();
        }
        
        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                puzzleRunning = 0;
                game->currentState = MENU_STATE;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                puzzleRunning = 0;
                game->currentState = MENU_STATE;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (pointInRect(mouse, helpButtonRect)) {
                    game->currentState = PUZZLE_HELP_STATE;
                    if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                    puzzleRunning = 0;
                    break;
                }
                
                if (!showResult) {
                    if (pointInRect(mouse, answerRects[2])) {
                        result = 1;
                        showResult = 1;
                        resultStartTime = SDL_GetTicks();
                        game->player.score += 200;
                        if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                    }
                    else if (pointInRect(mouse, answerRects[0]) || pointInRect(mouse, answerRects[1])) {
                        result = 0;
                        showResult = 1;
                        resultStartTime = SDL_GetTicks();
                        if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                    }
                }
            }
        }

        if (game->currentState == PUZZLE_STATE) {
            // Draw background
            SDL_BlitSurface(puzzleBg, NULL, game->screen, NULL);
            
            // Draw timer
            if (!showResult) {
                drawTimer(game, progress);
            }

            // Draw answer choices
            for (int i = 0; i < 3; i++) {
                if (pointInRect(mouse, answerRects[i]) && !showResult) {
                    SDL_Rect highlight = answerRects[i];
                    highlight.x -= 5;
                    highlight.y -= 5;
                    highlight.w += 10;
                    highlight.h += 10;
                    SDL_FillRect(game->screen, &highlight, SDL_MapRGB(game->screen->format, 255, 255, 0));
                }
                
                SDL_BlitSurface(answers[i], NULL, game->screen, &answerRects[i]);
            }

            // Draw help button
            SDL_BlitSurface(game->help_btn, NULL, game->screen, &helpButtonRect);

            // Show result
            if (showResult) {
                SDL_Surface* resultImg = result ? game->successImg : game->failureImg;
                SDL_Rect resultRect = {
                    WINDOW_WIDTH/2 - resultImg->w/2,
                    WINDOW_HEIGHT/2 - resultImg->h/2,
                    resultImg->w,
                    resultImg->h
                };
                SDL_BlitSurface(resultImg, NULL, game->screen, &resultRect);

                if (SDL_GetTicks() - resultStartTime > 2000) {
                    puzzleRunning = 0;
                }
            }

            SDL_Flip(game->screen);
        }
        SDL_Delay(10);
    }

    // Free the loaded images for this puzzle set
    SDL_FreeSurface(puzzleBg);
    for (int i = 0; i < 3; i++) {
        SDL_FreeSurface(answers[i]);
    }

    if (game->currentState == PUZZLE_STATE) {
        game->currentState = MENU_STATE;
    }
    if (game->bgMusic) Mix_PlayMusic(game->bgMusic, -1);
}
void runPuzzleHelpSystem(GameData *game) {
    // Calculate help screen position (centered)
    SDL_Rect helpScreenRect = {
        (WINDOW_WIDTH - HELP_WIDTH)/2,
        (WINDOW_HEIGHT - HELP_HEIGHT)/2,
        HELP_WIDTH,
        HELP_HEIGHT
    };

    // Button positions (relative to help screen)
    SDL_Rect nextButtonRect = {HELP_WIDTH-110, HELP_HEIGHT-60, 100, 50};
    SDL_Rect prevButtonRect = {10, HELP_HEIGHT-60, 100, 50};
    SDL_Rect exitButtonRect = {HELP_WIDTH/2-50, HELP_HEIGHT-60, 100, 50};

    while (game->currentState == PUZZLE_HELP_STATE) {
        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        
        // Adjust mouse coordinates relative to help screen
        int helpX = mouse.x - helpScreenRect.x;
        int helpY = mouse.y - helpScreenRect.y;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game->currentState = GAMEOVER_STATE;
                return;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                game->currentState = PUZZLE_STATE;
                return;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Check navigation buttons
                switch(game->help_state) {
                    case HELP_SCREEN4:
                        if (pointInRect((Point){helpX, helpY}, nextButtonRect)) {
                            game->help_state = HELP_SCREEN5;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        break;
                        
                    case HELP_SCREEN5:
                        if (pointInRect((Point){helpX, helpY}, prevButtonRect)) {
                            game->help_state = HELP_SCREEN4;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        else if (pointInRect((Point){helpX, helpY}, nextButtonRect)) {
                            game->help_state = HELP_SCREEN6;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        break;
                        
                    case HELP_SCREEN6:
                        if (pointInRect((Point){helpX, helpY}, prevButtonRect)) {
                            game->help_state = HELP_SCREEN5;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        else if (pointInRect((Point){helpX, helpY}, exitButtonRect)) {
                            game->currentState = PUZZLE_STATE;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                            return;
                        }
                        break;
                        
                    default:
                        break;
                }
            }
        }

        // Rendering
        SDL_FillRect(game->screen, NULL, SDL_MapRGB(game->screen->format, 0, 0, 0));
        SDL_BlitSurface(game->puzzleBg, NULL, game->screen, NULL);
        
        // Draw overlay
        SDL_BlitSurface(game->overlay, NULL, game->screen, NULL);
        
        // Draw help screen background
        SDL_FillRect(game->screen, &helpScreenRect, SDL_MapRGB(game->screen->format, 255, 255, 255));
        
        // Draw current help screen
        SDL_Surface *currentHelp = NULL;
        switch(game->help_state) {
            case HELP_SCREEN4: currentHelp = game->help4_img; break;
            case HELP_SCREEN5: currentHelp = game->help5_img; break;
            case HELP_SCREEN6: currentHelp = game->help6_img; break;
            default: break;
        }
        
        if (currentHelp) {
            SDL_BlitSurface(currentHelp, NULL, game->screen, &helpScreenRect);
            
            // Draw navigation buttons
            switch(game->help_state) {
                case HELP_SCREEN4:
                    SDL_BlitSurface(game->next_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + nextButtonRect.x,
                                      helpScreenRect.y + nextButtonRect.y,
                                      nextButtonRect.w,
                                      nextButtonRect.h
                                  });
                    break;
                    
                case HELP_SCREEN5:
                    SDL_BlitSurface(game->prev_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + prevButtonRect.x,
                                      helpScreenRect.y + prevButtonRect.y,
                                      prevButtonRect.w,
                                      prevButtonRect.h
                                  });
                    SDL_BlitSurface(game->next_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + nextButtonRect.x,
                                      helpScreenRect.y + nextButtonRect.y,
                                      nextButtonRect.w,
                                      nextButtonRect.h
                                  });
                    break;
                    
                case HELP_SCREEN6:
                    SDL_BlitSurface(game->prev_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + prevButtonRect.x,
                                      helpScreenRect.y + prevButtonRect.y,
                                      prevButtonRect.w,
                                      prevButtonRect.h
                                  });
                    SDL_BlitSurface(game->exit_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + exitButtonRect.x,
                                      helpScreenRect.y + exitButtonRect.y,
                                      exitButtonRect.w,
                                      exitButtonRect.h
                                  });
                    break;
                    
                default:
                    break;
            }
        }

        SDL_Flip(game->screen);
        SDL_Delay(10);
    }
}
void runQuizGame(GameData *game) {
    Mix_HaltChannel(-1);

    // Color definitions
    const SDL_Color black = {0, 0, 0, 255};
    const SDL_Color green = {0, 255, 0, 255};
    const SDL_Color red = {255, 0, 0, 255};
    const SDL_Color gold = {255, 215, 0, 255};
    const SDL_Color progressGreen = {0, 200, 0, 255};
    const SDL_Color progressBgColor = {50, 50, 50, 255};

    // Calculate button positions
    SDL_Rect questionRect = {
        (WINDOW_WIDTH - BUTTON_WIDTH)/2 - 60, 
        100, 
        BUTTON_WIDTH, 
        BUTTON_HEIGHT
    };
    
    SDL_Rect answerRects[3];
    const int answerSpacing = 20;
    const int totalWidth = 3*BUTTON_WIDTH + 2*answerSpacing;
    const int startX = (WINDOW_WIDTH - totalWidth)/2;
    
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

    // Help button position (top right corner)
    SDL_Rect helpButtonRect = {
        WINDOW_WIDTH - game->help_btn->w - 20, 
        20, 
        game->help_btn->w, 
        game->help_btn->h
    };

    // Initialize game state
    srand(time(NULL));
    int currentQuestion = rand() % game->questionCount;
    int selectedAnswer = -1;
    int showingAnswer = 0;
    int running = 1;
    int showNextButton = 0;
    Uint32 startTime = SDL_GetTicks();
    int lastHoveredAnswer = -1;
    int currentEmotion = 0;
    int currentFrame = 0;
    Uint32 lastFrameTime = SDL_GetTicks();
    const Uint32 frameDelay = 200;

    // Main game loop
    while (running) {
        // Handle frame animation
        if (SDL_GetTicks() - lastFrameTime > frameDelay) {
            currentFrame = (currentFrame + 1) % 3;
            lastFrameTime = SDL_GetTicks();
        }

        // Calculate remaining time
        float timeLeft = TIMER_DURATION - (SDL_GetTicks() - startTime)/1000.0f;
        float progress = timeLeft/TIMER_DURATION;
        
        // Time's up handling
        if (timeLeft <= 0 && !showingAnswer) {
            showingAnswer = 1;
            selectedAnswer = -1;
            game->player.lives--;
            currentEmotion = 3;
            showNextButton = 1;
            
            if (game->player.lives <= 0) {
                game->currentState = GAMEOVER_STATE;
                break;
            }
        }
        
        // Mouse input handling
        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        int hoveredAnswer = -1;
        
        // Handle answer hover effects
        if (!showingAnswer) {
            currentEmotion = 0;
            for (int i = 0; i < 3; i++) {
                if (pointInRect(mouse, answerRects[i])) {
                    currentEmotion = 1;
                    hoveredAnswer = i;
                    
                    if (hoveredAnswer != lastHoveredAnswer && game->hoverSound) {
                        Mix_PlayChannel(-1, game->hoverSound, 0);
                    }
                    break;
                }
            }
            lastHoveredAnswer = hoveredAnswer;
        }
        
        // Event processing
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game->currentState = GAMEOVER_STATE;
                running = 0;
                break;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                game->currentState = MENU_STATE;
                running = 0;
                break;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                Point mouse = {event.button.x, event.button.y};
                
                // Check if help button was clicked
                if (pointInRect(mouse, helpButtonRect)) {
                    game->currentState = HELP_STATE;
                    running = 0;
                    break;
                }
                
                // Answer selection
                if (!showingAnswer) {
                    for (int i = 0; i < 3; i++) {
                        if (pointInRect(mouse, answerRects[i])) {
                            selectedAnswer = i;
                            showingAnswer = 1;
                            showNextButton = 1;
                            
                            if (game->clickSound) {
                                Mix_PlayChannel(-1, game->clickSound, 0);
                            }
                            
                            // Check if answer is correct
                            if (selectedAnswer == game->questions[currentQuestion].correctAnswer) {
                                currentEmotion = 2;
                                game->player.score += 100 * game->player.level;
                                game->player.questionsSurvived++;
                                
                                if (game->player.questionsSurvived >= QUESTIONS_TO_WIN) {
                                    game->currentState = WIN_STATE;
                                    running = 0;
                                    break;
                                }
                            } else {
                                currentEmotion = 3;
                                game->player.lives--;
                                
                                if (game->player.lives <= 0) {
                                    game->currentState = GAMEOVER_STATE;
                                    running = 0;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                // Next question
                else if (showNextButton && pointInRect(mouse, nextButtonRect)) {
                    currentEmotion = 0;
                    currentQuestion = rand() % game->questionCount;
                    selectedAnswer = -1;
                    showingAnswer = 0;
                    showNextButton = 0;
                    startTime = SDL_GetTicks();
                }
            }
        }

        // Rendering
        SDL_FillRect(game->screen, NULL, SDL_MapRGB(game->screen->format, 0, 0, 0));
        SDL_BlitSurface(game->gameBg, NULL, game->screen, NULL);
        
        // Draw timer if not showing answer
        if (!showingAnswer) {
            drawTimer(game, progress);
        }
        
        // Draw emotion animation
        SDL_Rect emotionRect = {0, 0, 100, 100};
        if (game->emotionImgs[currentEmotion][currentFrame]) {
            SDL_BlitSurface(game->emotionImgs[currentEmotion][currentFrame], NULL, game->screen, &emotionRect);
        }

        // Draw question
        SDL_BlitSurface(game->questionButton, NULL, game->screen, &questionRect);
        renderCenteredText(game->screen, game->font, game->questions[currentQuestion].question, questionRect, black);
        
        // Draw answers
        for (int i = 0; i < 3; i++) {
            // Choose button image (normal or hover)
            SDL_Surface *button = (hoveredAnswer == i && !showingAnswer && game->answerButtonsHover[i]) ? 
                                 game->answerButtonsHover[i] : game->answerButtons[i];
            
            if (button) {
                SDL_BlitSurface(button, NULL, game->screen, &answerRects[i]);
            }
            
            // Set answer text color
            SDL_Color textColor = black;
            if (showingAnswer) {
                if (i == game->questions[currentQuestion].correctAnswer) {
                    textColor = green;  // Correct answer
                } else if (i == selectedAnswer) {
                    textColor = red;    // Wrong answer
                }
            }
            renderCenteredText(game->screen, game->font, game->questions[currentQuestion].answers[i], answerRects[i], textColor);
        }

        // Draw next button if needed
        if (showNextButton && game->nextButton) {
            SDL_BlitSurface(game->nextButton, NULL, game->screen, &nextButtonRect);
            renderCenteredText(game->screen, game->font, "Next", nextButtonRect, gold);
        }

        // Draw help button (top right corner)
        SDL_BlitSurface(game->help_btn, NULL, game->screen, &helpButtonRect);

        // Draw player stats
        char statsText[128];
        SDL_Rect statsRect = {20, 180, 300, 100};
        sprintf(statsText, "Lives: %d\nLevel: %d\nScore: %d", 
                game->player.lives, game->player.level, game->player.score);
        SDL_Surface *statsSurface = TTF_RenderText_Blended(game->font, statsText, gold);
        if (statsSurface) {
            SDL_BlitSurface(statsSurface, NULL, game->screen, &statsRect);
            SDL_FreeSurface(statsSurface);
        }

        // Draw progress bar background
        SDL_Rect progressBg = {10, WINDOW_HEIGHT - 40, WINDOW_WIDTH - 20, 20};
        SDL_FillRect(game->screen, &progressBg, SDL_MapRGB(game->screen->format, 
                     progressBgColor.r, progressBgColor.g, progressBgColor.b));
        
        // Calculate and draw progress bar
        int progressWidth = (int)((WINDOW_WIDTH - 20) * ((float)game->player.questionsSurvived / QUESTIONS_TO_WIN));
        progressWidth = progressWidth < 0 ? 0 : progressWidth;
        progressWidth = progressWidth > WINDOW_WIDTH - 20 ? WINDOW_WIDTH - 20 : progressWidth;
        
        SDL_Rect progressBar = {10, WINDOW_HEIGHT - 40, progressWidth, 20};
        SDL_FillRect(game->screen, &progressBar, SDL_MapRGB(game->screen->format, 
                     progressGreen.r, progressGreen.g, progressGreen.b));
        
        // Draw progress text
        char progressText[50];
        sprintf(progressText, "%d/%d", game->player.questionsSurvived, QUESTIONS_TO_WIN);
        SDL_Surface* progressTextSurface = TTF_RenderText_Blended(game->font, progressText, gold);
        if (progressTextSurface) {
            SDL_Rect textRect = {
                (WINDOW_WIDTH - progressTextSurface->w)/2,
                WINDOW_HEIGHT - 40,
                progressTextSurface->w,
                progressTextSurface->h
            };
            SDL_BlitSurface(progressTextSurface, NULL, game->screen, &textRect);
            SDL_FreeSurface(progressTextSurface);
        }

        // Update screen
        SDL_Flip(game->screen);
        SDL_Delay(10);
    }
}

void runHelpSystem(GameData *game) {
    // Calculate help screen position (centered)
    SDL_Rect helpScreenRect = {
        (WINDOW_WIDTH - HELP_WIDTH)/2,
        (WINDOW_HEIGHT - HELP_HEIGHT)/2,
        HELP_WIDTH,
        HELP_HEIGHT
    };

    // Button positions (relative to help screen)
    SDL_Rect nextButtonRect = {HELP_WIDTH-110, HELP_HEIGHT-60, 100, 50};
    SDL_Rect prevButtonRect = {10, HELP_HEIGHT-60, 100, 50};
    SDL_Rect exitButtonRect = {HELP_WIDTH/2-50, HELP_HEIGHT-60, 100, 50};

    // Main help system loop
    while (game->currentState == HELP_STATE) {
        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        
        // Adjust mouse coordinates relative to help screen
        int helpX = mouse.x - helpScreenRect.x;
        int helpY = mouse.y - helpScreenRect.y;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game->currentState = GAMEOVER_STATE;
                return;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                game->currentState = GAME_STATE;
                return;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Check navigation buttons
                switch(game->help_state) {
                    case HELP_SCREEN1:
                        if (pointInRect((Point){helpX, helpY}, nextButtonRect)) {
                            game->help_state = HELP_SCREEN2;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        break;
                        
                    case HELP_SCREEN2:
                        if (pointInRect((Point){helpX, helpY}, prevButtonRect)) {
                            game->help_state = HELP_SCREEN1;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        else if (pointInRect((Point){helpX, helpY}, nextButtonRect)) {
                            game->help_state = HELP_SCREEN3;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        break;
                        
                    case HELP_SCREEN3:
                        if (pointInRect((Point){helpX, helpY}, prevButtonRect)) {
                            game->help_state = HELP_SCREEN2;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                        }
                        else if (pointInRect((Point){helpX, helpY}, exitButtonRect)) {
                            game->currentState = GAME_STATE;
                            if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                            return;
                        }
                        break;
                        
                    default:
                        break;
                }
            }
        }

        // Rendering
        SDL_FillRect(game->screen, NULL, SDL_MapRGB(game->screen->format, 0, 0, 0));
        SDL_BlitSurface(game->gameBg, NULL, game->screen, NULL);
        
        // Draw overlay
        SDL_BlitSurface(game->overlay, NULL, game->screen, NULL);
        
        // Draw help screen background
        SDL_FillRect(game->screen, &helpScreenRect, SDL_MapRGB(game->screen->format, 255, 255, 255));
        
        // Draw current help screen
        SDL_Surface *currentHelp = NULL;
        switch(game->help_state) {
            case HELP_SCREEN1: currentHelp = game->help1_img; break;
            case HELP_SCREEN2: currentHelp = game->help2_img; break;
            case HELP_SCREEN3: currentHelp = game->help3_img; break;
            default: break;
        }
        
        if (currentHelp) {
            SDL_BlitSurface(currentHelp, NULL, game->screen, &helpScreenRect);
            
            // Draw navigation buttons
            switch(game->help_state) {
                case HELP_SCREEN1:
                    SDL_BlitSurface(game->next_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + nextButtonRect.x,
                                      helpScreenRect.y + nextButtonRect.y,
                                      nextButtonRect.w,
                                      nextButtonRect.h
                                  });
                    break;
                    
                case HELP_SCREEN2:
                    SDL_BlitSurface(game->prev_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + prevButtonRect.x,
                                      helpScreenRect.y + prevButtonRect.y,
                                      prevButtonRect.w,
                                      prevButtonRect.h
                                  });
                    SDL_BlitSurface(game->next_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + nextButtonRect.x,
                                      helpScreenRect.y + nextButtonRect.y,
                                      nextButtonRect.w,
                                      nextButtonRect.h
                                  });
                    break;
                    
                case HELP_SCREEN3:
                    SDL_BlitSurface(game->prev_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + prevButtonRect.x,
                                      helpScreenRect.y + prevButtonRect.y,
                                      prevButtonRect.w,
                                      prevButtonRect.h
                                  });
                    SDL_BlitSurface(game->exit_btn, NULL, game->screen, 
                                  &(SDL_Rect){
                                      helpScreenRect.x + exitButtonRect.x,
                                      helpScreenRect.y + exitButtonRect.y,
                                      exitButtonRect.w,
                                      exitButtonRect.h
                                  });
                    break;
                    
                default:
                    break;
            }
        }

        SDL_Flip(game->screen);
        SDL_Delay(10);
    }
}

void runMenu(GameData *game) {
    SDL_Rect quizButtonRect = {
        (game->screen->w / 2) - 200,
        (game->screen->h / 2) - 200,
        game->quizButton->w,
        game->quizButton->h
    };
    
    SDL_Rect puzzleButtonRect = {
        (game->screen->w / 2) + 50,
        quizButtonRect.y,
        game->puzzleButton->w,
        game->puzzleButton->h
    };

    if (game->bgMusic) {
        Mix_PlayMusic(game->bgMusic, -1);
    }

    int wasHovering = 0;

    while (game->currentState == MENU_STATE) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game->currentState = GAMEOVER_STATE;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    game->currentState = GAMEOVER_STATE;
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                Point mouse = {event.button.x, event.button.y};
                if (pointInRect(mouse, quizButtonRect)) {
                    if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                    if (game->bgMusic) Mix_HaltMusic();
                    game->currentState = GAME_STATE;
                    game->player = (PlayerStats){0, 3, 1, 0, 0};
                }
                else if (pointInRect(mouse, puzzleButtonRect)) {
                    if (game->clickSound) Mix_PlayChannel(-1, game->clickSound, 0);
                    if (game->bgMusic) Mix_HaltMusic();
                    game->currentState = PUZZLE_STATE;
                }
            }
        }

        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        int isHoveringQuiz = pointInRect(mouse, quizButtonRect);
        int isHoveringPuzzle = pointInRect(mouse, puzzleButtonRect);
        int isHovering = isHoveringQuiz || isHoveringPuzzle;

        // Play hover sound only when first entering hover state
        if (isHovering && !wasHovering && game->hoverSound) {
            Mix_PlayChannel(-1, game->hoverSound, 0);
        }
        wasHovering = isHovering;

        SDL_BlitSurface(game->menuBg, NULL, game->screen, NULL);
        SDL_BlitSurface(isHoveringQuiz ? game->quizButtonHover : game->quizButton, 
                       NULL, game->screen, &quizButtonRect);
        SDL_BlitSurface(isHoveringPuzzle ? game->puzzleButtonHover : game->puzzleButton, 
                       NULL, game->screen, &puzzleButtonRect);
        
        SDL_Flip(game->screen);
        SDL_Delay(10);
    }
}

// Main function
int main() {
    GameData game = {0};
    
    if (!initGame(&game)) {
        cleanupGame(&game);
        return 1;
    }

    while (game.currentState != GAMEOVER_STATE) {
        switch (game.currentState) {
            case MENU_STATE:
                runMenu(&game);
                break;
            case GAME_STATE:
                game.help_state = HELP_SCREEN1; // Reset help screen when entering quiz
                runQuizGame(&game);
                break;
            case PUZZLE_STATE:
                game.help_state = HELP_SCREEN4; // Reset help screen when entering puzzle
                runPuzzleGame(&game);
                break;
            case HELP_STATE:
                runHelpSystem(&game);
                break;
            case PUZZLE_HELP_STATE:
                runPuzzleHelpSystem(&game);
                break;
            case WIN_STATE:
                if (game.winMusic) Mix_PlayMusic(game.winMusic, 0);
                showWinScreen(&game);
                game.currentState = MENU_STATE;
                if (game.bgMusic) Mix_PlayMusic(game.bgMusic, -1);
                break;
            case GAMEOVER_STATE:
                if (game.gameOverMusic) Mix_PlayMusic(game.gameOverMusic, 0);
                showGameOver(&game);
                game.currentState = MENU_STATE;
                if (game.bgMusic) Mix_PlayMusic(game.bgMusic, -1);
                break;
        }
    }

    cleanupGame(&game);
    return 0;
}
