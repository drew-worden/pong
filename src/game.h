#pragma once

// Includes
#include "raylib.h"

// Constants
// Logical resolution of the playfield (4:3 like the original). The game is
// rendered at this size and then scaled to fit the window with letterboxing.
#define GAME_WIDTH  800
#define GAME_HEIGHT 600

#define BACKGROUND_COLOR CLITERAL(Color){ 0x25, 0x25, 0x25, 0xFF }

// Types
// High-level state machine for the game loop.
typedef enum {
    STATE_MENU,       // Player-count selection screen
    STATE_SERVE,      // Brief pause with the ball centered before it is released
    STATE_PLAY,       // Ball in play
    STATE_PAUSED,     // Play suspended; resumes to STATE_PLAY
    STATE_GAME_OVER,  // A player reached the winning score
} GameState;

typedef struct {
    Rectangle rect;
    int score;
} Paddle;

typedef struct {
    Rectangle rect;
    Vector2 velocity;  // Game units per second
} Ball;

typedef struct {
    GameState state;
    int players;         // 1 = left paddle vs CPU, 2 = two human players
    int menuSelection;   // Highlighted menu row (0-based)
    Paddle left;
    Paddle right;
    Ball ball;
    float serveTimer;    // Seconds remaining in STATE_SERVE
    int serveDirection;  // -1 serves toward the left paddle, 1 toward the right
} Game;

// Functions
// Resets the game to the menu with paddles centered.
void GameInit(Game *game);

// Advances the simulation by dt seconds, reading keyboard and gamepad input.
void GameUpdate(Game *game, float dt);

// Draws the current state into the active render target using game coordinates.
void GameDraw(const Game *game);
