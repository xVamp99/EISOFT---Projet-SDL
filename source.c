#include "headers.h"

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

    SDL_FreeSurface(gameOverBg);
}

void showWinScreen(SDL_Surface *screen, TTF_Font *font) {
    SDL_Surface *winBg = loadImage("win.png");
    if (!winBg) {
        printf("Failed to load win background\n");
        return;
    }

    if (enigmeMusic) Mix_HaltMusic();
    if (winMusic) Mix_PlayMusic(winMusic, 0);

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

    if (winMusic) Mix_HaltMusic();
    if (enigmeMusic) Mix_PlayMusic(enigmeMusic, -1);

    SDL_FreeSurface(winBg);
}

void runQuizGame(SDL_Surface *screen, TTF_Font *font, QuizQuestion *questions, int questionCount) {
    SDL_Surface *bg = loadImage("2D.png");
    SDL_Surface *questionButton = loadImage("button.png");
    SDL_Surface *answerButtons[3] = {NULL};
    SDL_Surface *answerButtonsHover[3] = {NULL};
    SDL_Surface *nextButton = loadImage("next.png");
    
    answerButtons[0] = loadImage("a.png");
    answerButtons[1] = loadImage("b.png");
    answerButtons[2] = loadImage("c.png");
    
    answerButtonsHover[0] = loadImage("a1.png");
    answerButtonsHover[1] = loadImage("b1.png");
    answerButtonsHover[2] = loadImage("c1.png");
    
    SDL_Rect questionRect = {(WINDOW_WIDTH-BUTTON_WIDTH)/2, QUESTION_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    int totalWidth = 3*BUTTON_WIDTH + 2*BUTTON_SPACING;
    int startX = (WINDOW_WIDTH - totalWidth)/2;
    SDL_Rect answerRects[3];
    for (int i = 0; i < 3; i++) {
        answerRects[i] = (SDL_Rect){startX + i*(BUTTON_WIDTH+BUTTON_SPACING), ANSWERS_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
    }

    SDL_Rect nextButtonRect = {
        (WINDOW_WIDTH - NEXT_BUTTON_WIDTH)/2,
        ANSWERS_Y + BUTTON_HEIGHT + 50,
        NEXT_BUTTON_WIDTH,
        NEXT_BUTTON_HEIGHT
    };

    SDL_Color black = {0, 0, 0, 0};
    SDL_Color white = {255, 255, 255, 0};
    SDL_Color green = {0, 255, 0, 0};
    SDL_Color red = {255, 0, 0, 0};
    SDL_Color gold = {255, 215, 0, 0};

    srand(time(NULL));
    int currentQuestion = rand() % questionCount;
    int selectedAnswer = -1;
    int showingAnswer = 0;
    int running = 1;
    int showNextButton = 0;
    
    Uint32 startTime = SDL_GetTicks();
    Uint32 currentTime;
    float timeLeft;
    float progress;

    while (running) {
        currentTime = SDL_GetTicks();
        timeLeft = TIMER_DURATION - (currentTime - startTime)/1000.0f;
        progress = timeLeft/TIMER_DURATION;
        
        if (timeLeft <= 0 && !showingAnswer) {
            showingAnswer = 1;
            selectedAnswer = -1;
            shouldBeep = 0;
            player.lives--;
            player.consecutiveCorrect = 0;
            showNextButton = 1;
            
            if (player.lives <= 0) {
                currentState = GAMEOVER_STATE;
                running = 0;
            }
        }
        
        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        int hoveredAnswer = -1;
        if (!showingAnswer) {
            for (int i = 0; i < 3; i++) {
                if (pointInRect(mouse, answerRects[i])) {
                    hoveredAnswer = i;
                    static int lastHovered = -1;
                    if (hoveredAnswer != lastHovered && sourisSound) {
                        Mix_PlayChannel(-1, sourisSound, 0);
                    }
                    lastHovered = hoveredAnswer;
                    break;
                }
            }
        }
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                currentState = MENU_STATE;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                    currentState = MENU_STATE;
                }
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
                                int timeBonus = (int)(timeLeft * 2);
                                player.score += 100 * player.level + timeBonus;
                                player.consecutiveCorrect++;
                                player.questionsSurvived++;
                                
                                if (player.consecutiveCorrect >= 3) {
                                    player.score += 50 * player.level;
                                }
                                
                                if (player.score >= player.level * 500) {
                                    player.level++;
                                    player.lives++;
                                }

                                if (player.questionsSurvived >= QUESTIONS_TO_WIN) {
                                    currentState = WIN_STATE;
                                    running = 0;
                                    break;
                                }
                            } else {
                                player.score -= 50 * player.level;
                                if (player.score < 0) player.score = 0;
                                player.lives--;
                                player.consecutiveCorrect = 0;
                                
                                if (player.lives <= 0) {
                                    currentState = GAMEOVER_STATE;
                                    running = 0;
                                }
                            }
                            break;
                        }
                    }
                } else if (showNextButton && pointInRect(mouse, nextButtonRect)) {
                    int channel = Mix_PlayChannel(-1, hoverSound, 0);
                    Uint32 soundStart = SDL_GetTicks();
                    
                    while (SDL_GetTicks() - soundStart < 1000) {
                        SDL_PumpEvents();
                    }
                    Mix_HaltChannel(channel);
                    
                    currentQuestion = rand() % questionCount;
                    selectedAnswer = -1;
                    showingAnswer = 0;
                    showNextButton = 0;
                    startTime = SDL_GetTicks();
                    shouldBeep = 1;
                    lastBeepTime = SDL_GetTicks();
                }
            }
        }
        
        updateBeepSound(progress);

        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
        SDL_BlitSurface(bg, NULL, screen, NULL);
        
        if (!showingAnswer) {
            drawTimer(screen, progress);
        }
        
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
                    renderCenteredText(screen, font, "✓", 
                        (SDL_Rect){answerRects[i].x + BUTTON_WIDTH - 30, 
                                  answerRects[i].y + 10, 20, 20}, green);
                } else if (i == selectedAnswer) {
                    textColor = red;
                    renderCenteredText(screen, font, "✗", 
                        (SDL_Rect){answerRects[i].x + BUTTON_WIDTH - 30, 
                                  answerRects[i].y + 10, 20, 20}, red);
                }
            }
            renderCenteredText(screen, font, questions[currentQuestion].answers[i], answerRects[i], textColor);
        }

        if (showNextButton && nextButton) {
            SDL_BlitSurface(nextButton, NULL, screen, &nextButtonRect);
            renderCenteredText(screen, font, "Next", nextButtonRect, gold);
        }

        renderPlayerStats(screen, font);
        renderCenteredText(screen, font, "chow your limit", (SDL_Rect){0, 20, WINDOW_WIDTH, 40}, white);
        
        SDL_Flip(screen);
        SDL_Delay(10);
    }

    SDL_FreeSurface(bg);
    SDL_FreeSurface(questionButton);
    for (int i = 0; i < 3; i++) {
        SDL_FreeSurface(answerButtons[i]);
        if (answerButtonsHover[i]) SDL_FreeSurface(answerButtonsHover[i]);
    }
    if (nextButton) SDL_FreeSurface(nextButton);
}
