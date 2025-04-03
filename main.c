#include "headers.h"

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

int main() {
    SDL_Surface *screen = NULL;
    TTF_Font *font = NULL;
    
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
    
    if (!beepSound || !hoverSound || !sourisSound) {
        printf("Warning: Could not load sound effects\n");
    }
    if (!enigmeMusic) {
        printf("Warning: Could not load enigme music\n");
    }
    if (!winMusic) {
        printf("Warning: Could not load win music\n");
    }
    
    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        printf("TTF_OpenFont failed: %s\n", TTF_GetError());
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
    
    if (!menuBg || !quizButton || !quizButtonHover || !puzzleButton || !puzzleButtonHover) {
        printf("Failed to load menu assets\n");
        return 1;
    }

    SDL_Rect quizButtonRect = {
        (screen->w / 2) - 100,
        (screen->h / 2) - 100,
        quizButton->w,
        quizButton->h
    };
    
    SDL_Rect puzzleButtonRect = {
        (screen->w / 2) + 50,
        quizButtonRect.y,
        puzzleButton->w,
        puzzleButton->h
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
                }
            }
        }

        if (currentState == MENU_STATE) {
            Point mouse;
            SDL_GetMouseState(&mouse.x, &mouse.y);
            int isHoveringQuiz = pointInRect(mouse, quizButtonRect);
            int isHoveringPuzzle = pointInRect(mouse, puzzleButtonRect);

            if ((isHoveringQuiz || isHoveringPuzzle) && lastHoveredButton == -1) {
                if (sourisSound) {
                    Mix_PlayChannel(-1, sourisSound, 0);
                }
            }
            
            if (isHoveringQuiz) {
                lastHoveredButton = 0;
            } else if (isHoveringPuzzle) {
                lastHoveredButton = 1;
            } else {
                lastHoveredButton = -1;
            }

            SDL_BlitSurface(menuBg, NULL, screen, NULL);
            SDL_BlitSurface(isHoveringQuiz ? quizButtonHover : quizButton, NULL, screen, &quizButtonRect);
            SDL_BlitSurface(isHoveringPuzzle ? puzzleButtonHover : puzzleButton, NULL, screen, &puzzleButtonRect);
            
            SDL_Flip(screen);
        }
        else if (currentState == GAME_STATE) {
            runQuizGame(screen, font, questions, questionCount);
            if (currentState == GAMEOVER_STATE) {
                showGameOver(screen, font);
                currentState = MENU_STATE;
                if (enigmeMusic) Mix_PlayMusic(enigmeMusic, -1);
            }
            else if (currentState == WIN_STATE) {
                showWinScreen(screen, font);
                currentState = MENU_STATE;
            }
        }

        SDL_Delay(10);
    }

    if (beepSound) Mix_FreeChunk(beepSound);
    if (hoverSound) Mix_FreeChunk(hoverSound);
    if (sourisSound) Mix_FreeChunk(sourisSound);
    if (enigmeMusic) Mix_FreeMusic(enigmeMusic);
    if (winMusic) Mix_FreeMusic(winMusic);
    Mix_CloseAudio();
    SDL_FreeSurface(menuBg);
    SDL_FreeSurface(quizButton);
    SDL_FreeSurface(quizButtonHover);
    SDL_FreeSurface(puzzleButton);
    SDL_FreeSurface(puzzleButtonHover);
    TTF_CloseFont(font);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
