// Includes
#include "game.h"
#include "audio.h"
#include "font.h"
#include <math.h>

// Constants
#define PADDLE_W 12
#define PADDLE_H 80
#define PADDLE_MARGIN 40
#define PADDLE_SPEED 450.0f
#define CPU_SPEED 330.0f
#define CPU_DEADZONE 6.0f

#define BALL_SIZE 12
#define BALL_SPEED 380.0f
#define BALL_MAX_SPEED 900.0f
#define BALL_SPEEDUP 1.06f
#define MAX_SERVE_ANGLE 30.0f
#define MAX_BOUNCE_ANGLE 60.0f

#define NET_W 4
#define NET_DASH 15
#define NET_GAP 15

#define WIN_SCORE 11
#define SERVE_DELAY 1.0f

#define SCORE_PIXEL 10
#define SCORE_OFFSET 60
#define SCORE_Y 30

#define OPTION_PIXEL 8
#define OPTION_SPACING 60

#define GAMEPAD_DEADZONE 0.2f

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
// Gamepad index maps directly to player index: pad 0 drives the left paddle,
// pad 1 the right. Menu/pause actions accept input from either pad so a single
// controller can navigate regardless of which port it occupies.

// A (bottom face button) or Start confirms menu selections.
static bool GamepadConfirmPressed(int pad) {
    return IsGamepadAvailable(pad) &&
           (IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
            IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}

static bool GamepadPausePressed(int pad) {
    return IsGamepadAvailable(pad) && IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_MIDDLE_RIGHT);
}

static bool ConfirmPressed(void) {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
           GamepadConfirmPressed(0) || GamepadConfirmPressed(1);
}

static bool PausePressed(void) {
    return IsKeyPressed(KEY_P) || GamepadPausePressed(0) || GamepadPausePressed(1);
}

// R or Select/Back abandons the current match and starts a new one.
static bool GamepadRestartPressed(int pad) {
    return IsGamepadAvailable(pad) && IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_MIDDLE_LEFT);
}

static bool RestartPressed(void) {
    return IsKeyPressed(KEY_R) || GamepadRestartPressed(0) || GamepadRestartPressed(1);
}

// Returns -1 (up), 1 (down) or 0 from a single d-pad press on the given pad.
static int GamepadMenuDirection(int pad) {
    if (!IsGamepadAvailable(pad)) return 0;
    if (IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_LEFT_FACE_UP)) return -1;
    if (IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) return 1;
    return 0;
}

static int MenuDirection(void) {
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) return -1;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) return 1;
    int dir = GamepadMenuDirection(0);
    return dir ? dir : GamepadMenuDirection(1);
}

// Vertical analog input in -1..1. The d-pad overrides the stick so it always
// yields full-speed movement, and small stick drift is ignored via a deadzone.
static float GamepadStickAxis(int pad) {
    if (!IsGamepadAvailable(pad)) return 0.0f;
    float axis = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);
    if (fabsf(axis) < GAMEPAD_DEADZONE) axis = 0.0f;
    if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_UP)) axis = -1.0f;
    if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) axis = 1.0f;
    return axis;
}

// Combined keyboard and gamepad input for a player (0 = left, 1 = right) in
// -1..1. Player 1 uses W/S; player 2 uses the arrow keys. In single-player mode
// the arrow keys also drive player 1 since nobody else needs them.
static float PlayerAxis(const Game *game, int player) {
    float axis = 0.0f;
    bool useArrows = (player == 1) || (game->players == 1);
    bool useWasd = (player == 0);

    if (useWasd) {
        if (IsKeyDown(KEY_W)) axis -= 1.0f;
        if (IsKeyDown(KEY_S)) axis += 1.0f;
    }
    if (useArrows) {
        if (IsKeyDown(KEY_UP)) axis -= 1.0f;
        if (IsKeyDown(KEY_DOWN)) axis += 1.0f;
    }
    axis += GamepadStickAxis(player);

    if (axis > 1.0f) axis = 1.0f;
    if (axis < -1.0f) axis = -1.0f;
    return axis;
}

// Simple AI for the right paddle: track the ball while it approaches, otherwise
// drift back to center. It moves slower than a human can and ignores small
// offsets so it is beatable and does not jitter.
static float CpuAxis(const Game *game) {
    float paddleCenter = game->right.rect.y + PADDLE_H / 2.0f;
    float target = GAME_HEIGHT / 2.0f;
    if (game->ball.velocity.x > 0) target = game->ball.rect.y + BALL_SIZE / 2.0f;

    float diff = target - paddleCenter;
    if (fabsf(diff) < CPU_DEADZONE) return 0.0f;
    return (diff > 0 ? 1.0f : -1.0f) * (CPU_SPEED / PADDLE_SPEED);
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

static float RandomRange(float min, float max) {
    return min + (max - min) * (GetRandomValue(0, 10000) / 10000.0f);
}

static void CenterPaddles(Game *game) {
    float y = (GAME_HEIGHT - PADDLE_H) / 2.0f;
    game->left.rect = (Rectangle){ PADDLE_MARGIN, y, PADDLE_W, PADDLE_H };
    game->right.rect = (Rectangle){ GAME_WIDTH - PADDLE_MARGIN - PADDLE_W, y, PADDLE_W, PADDLE_H };
}

// Places the ball at center court heading toward serveDirection at a random
// shallow angle, then holds it for SERVE_DELAY so players can get ready.
static void ServeBall(Game *game) {
    float angle = RandomRange(-MAX_SERVE_ANGLE, MAX_SERVE_ANGLE) * DEG2RAD;
    game->ball.rect = (Rectangle){ (GAME_WIDTH - BALL_SIZE) / 2.0f, (GAME_HEIGHT - BALL_SIZE) / 2.0f, BALL_SIZE, BALL_SIZE };
    game->ball.velocity = (Vector2){ cosf(angle) * BALL_SPEED * game->serveDirection, sinf(angle) * BALL_SPEED };
    game->serveTimer = SERVE_DELAY;
    game->state = STATE_SERVE;
}

static void ResetMatch(Game *game) {
    game->left.score = 0;
    game->right.score = 0;
    game->serveDirection = GetRandomValue(0, 1) ? 1 : -1;
    CenterPaddles(game);
    ServeBall(game);
}

static void MovePaddle(Paddle *paddle, float axis, float dt) {
    paddle->rect.y += axis * PADDLE_SPEED * dt;
    if (paddle->rect.y < 0) paddle->rect.y = 0;
    if (paddle->rect.y + PADDLE_H > GAME_HEIGHT) paddle->rect.y = GAME_HEIGHT - PADDLE_H;
}

static void UpdatePaddles(Game *game, float dt) {
    MovePaddle(&game->left, PlayerAxis(game, 0), dt);
    MovePaddle(&game->right, game->players == 2 ? PlayerAxis(game, 1) : CpuAxis(game), dt);
}

// Reflects the ball off a paddle. As in the original, the outgoing angle depends
// on where the ball struck relative to the paddle center (edges deflect more
// sharply) and each hit speeds the ball up until BALL_MAX_SPEED. The ball is
// also snapped flush against the paddle face so it cannot tunnel through.
static void BounceOffPaddle(Ball *ball, const Paddle *paddle, int direction) {
    float relative = ((ball->rect.y + BALL_SIZE / 2.0f) - (paddle->rect.y + PADDLE_H / 2.0f)) / (PADDLE_H / 2.0f);
    if (relative > 1.0f) relative = 1.0f;
    if (relative < -1.0f) relative = -1.0f;

    float angle = relative * MAX_BOUNCE_ANGLE * DEG2RAD;
    float speed = sqrtf(ball->velocity.x * ball->velocity.x + ball->velocity.y * ball->velocity.y) * BALL_SPEEDUP;
    if (speed > BALL_MAX_SPEED) speed = BALL_MAX_SPEED;

    ball->velocity = (Vector2){ cosf(angle) * speed * direction, sinf(angle) * speed };
    ball->rect.x = direction > 0 ? paddle->rect.x + PADDLE_W : paddle->rect.x - BALL_SIZE;
    AudioPlayPaddle();
}

// Awards a point and either ends the match or re-serves toward the player who
// conceded, giving them the next touch.
static void ScorePoint(Game *game, Paddle *scorer, int nextServeDirection) {
    scorer->score++;
    AudioPlayScore();

    if (scorer->score >= WIN_SCORE) {
        game->state = STATE_GAME_OVER;
        game->menuSelection = 0;
        return;
    }
    game->serveDirection = nextServeDirection;
    ServeBall(game);
}

static void UpdateBall(Game *game, float dt) {
    Ball *ball = &game->ball;
    ball->rect.x += ball->velocity.x * dt;
    ball->rect.y += ball->velocity.y * dt;

    if (ball->rect.y <= 0) {
        ball->rect.y = 0;
        ball->velocity.y = fabsf(ball->velocity.y);
        AudioPlayWall();
    } else if (ball->rect.y + BALL_SIZE >= GAME_HEIGHT) {
        ball->rect.y = GAME_HEIGHT - BALL_SIZE;
        ball->velocity.y = -fabsf(ball->velocity.y);
        AudioPlayWall();
    }

    // Only test the paddle the ball is moving toward so a ball that slips past
    // the paddle edge is not pulled back by a rear-side collision.
    if (ball->velocity.x < 0 && CheckCollisionRecs(ball->rect, game->left.rect)) {
        BounceOffPaddle(ball, &game->left, 1);
    } else if (ball->velocity.x > 0 && CheckCollisionRecs(ball->rect, game->right.rect)) {
        BounceOffPaddle(ball, &game->right, -1);
    }

    if (ball->rect.x + BALL_SIZE < 0) {
        ScorePoint(game, &game->right, -1);
    } else if (ball->rect.x > GAME_WIDTH) {
        ScorePoint(game, &game->left, 1);
    }
}

void GameInit(Game *game) {
    *game = (Game){ 0 };
    game->state = STATE_MENU;
    game->players = 1;
    CenterPaddles(game);
}

// Returns to the main menu with the current player count pre-selected.
static void ReturnToMenu(Game *game) {
    game->menuSelection = game->players - 1;
    game->state = STATE_MENU;
}

// Moves a two-row menu selection up or down, wrapping at both ends.
static void UpdateMenuSelection(Game *game) {
    int dir = MenuDirection();
    if (dir) game->menuSelection = (game->menuSelection + dir + 2) % 2;
}

void GameUpdate(Game *game, float dt) {
    // Restarting is allowed at any point during a match.
    if (game->state != STATE_MENU && game->state != STATE_GAME_OVER && RestartPressed()) {
        ResetMatch(game);
        return;
    }

    switch (game->state) {
        case STATE_MENU:
            UpdateMenuSelection(game);
            if (IsKeyPressed(KEY_ONE)) game->menuSelection = 0;
            if (IsKeyPressed(KEY_TWO)) game->menuSelection = 1;
            if (ConfirmPressed()) {
                game->players = game->menuSelection + 1;
                ResetMatch(game);
            }
            break;
        case STATE_SERVE:
            if (PausePressed()) { game->state = STATE_PAUSED; break; }
            UpdatePaddles(game, dt);
            game->serveTimer -= dt;
            if (game->serveTimer <= 0) game->state = STATE_PLAY;
            break;
        case STATE_PLAY:
            if (PausePressed()) { game->state = STATE_PAUSED; break; }
            UpdatePaddles(game, dt);
            UpdateBall(game, dt);
            break;
        case STATE_PAUSED:
            if (PausePressed() || ConfirmPressed()) game->state = STATE_PLAY;
            break;
        case STATE_GAME_OVER:
            UpdateMenuSelection(game);
            if (ConfirmPressed()) {
                if (game->menuSelection == 0) ResetMatch(game);
                else ReturnToMenu(game);
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

// Draws horizontally centered text over a background-colored strip so overlays
// such as "PAUSED" remain legible where they cross the net.
static void DrawCenteredText(const char *text, int y, int pixelSize, Color color) {
    int width = MeasurePixelText(text, pixelSize);
    int x = (GAME_WIDTH - width) / 2;
    int pad = pixelSize * 2;
    DrawRectangle(x - pad, y - pad, width + pad * 2, pixelSize * 5 + pad * 2, BACKGROUND_COLOR);
    DrawPixelText(text, x, y, pixelSize, color);
}

static void DrawNet(void) {
    for (int y = 0; y < GAME_HEIGHT; y += NET_DASH + NET_GAP) {
        DrawRectangle((GAME_WIDTH - NET_W) / 2, y, NET_W, NET_DASH, WHITE);
    }
}

// Scores sit at the top on either side of the net, as on the original cabinet.
// The left score is right-aligned so multi-digit values grow away from the net.
static void DrawScores(const Game *game) {
    const char *leftText = TextFormat("%d", game->left.score);
    const char *rightText = TextFormat("%d", game->right.score);
    int leftX = GAME_WIDTH / 2 - SCORE_OFFSET - MeasurePixelText(leftText, SCORE_PIXEL);
    int rightX = GAME_WIDTH / 2 + SCORE_OFFSET;
    DrawPixelText(leftText, leftX, SCORE_Y, SCORE_PIXEL, WHITE);
    DrawPixelText(rightText, rightX, SCORE_Y, SCORE_PIXEL, WHITE);
}

// Draws a vertical list of options with a ball-shaped cursor beside the
// selected row. Rows are centered and cleared behind so they read over the net.
static void DrawOptions(const char **options, int count, int y, int selected) {
    for (int i = 0; i < count; i++) {
        int rowY = y + i * OPTION_SPACING;
        bool isSelected = selected == i;
        DrawCenteredText(options[i], rowY, OPTION_PIXEL, isSelected ? WHITE : GRAY);
        if (isSelected) {
            int x = (GAME_WIDTH - MeasurePixelText(options[i], OPTION_PIXEL)) / 2;
            DrawRectangle(x - 40, rowY + 14, BALL_SIZE, BALL_SIZE, WHITE);
        }
    }
}

static void DrawMenu(const Game *game) {
    DrawCenteredText("PONG", 120, 20, WHITE);

    const char *options[] = { "1 PLAYER", "2 PLAYERS" };
    DrawOptions(options, 2, 330, game->menuSelection);

    DrawCenteredText("P1: W/S   P2: UP/DOWN   OR GAMEPAD", 500, 4, GRAY);
    DrawCenteredText("ENTER OR START TO PLAY   P TO PAUSE   R TO RESTART", 530, 4, GRAY);
}

static const char *WinnerText(const Game *game) {
    bool leftWon = game->left.score >= WIN_SCORE;
    if (game->players == 1) return leftWon ? "YOU WIN" : "CPU WINS";
    return leftWon ? "PLAYER 1 WINS" : "PLAYER 2 WINS";
}

void GameDraw(const Game *game) {
    ClearBackground(BACKGROUND_COLOR);

    if (game->state == STATE_MENU) {
        DrawMenu(game);
        return;
    }

    DrawNet();
    DrawScores(game);
    DrawRectangleRec(game->left.rect, WHITE);
    DrawRectangleRec(game->right.rect, WHITE);
    if (game->state != STATE_GAME_OVER) DrawRectangleRec(game->ball.rect, WHITE);

    if (game->state == STATE_PAUSED) {
        DrawCenteredText("PAUSED", 250, 10, WHITE);
        DrawCenteredText("P TO RESUME   R TO RESTART", 330, 4, GRAY);
    } else if (game->state == STATE_GAME_OVER) {
        DrawCenteredText(WinnerText(game), 200, 10, WHITE);
        const char *options[] = { "PLAY AGAIN", "MAIN MENU" };
        DrawOptions(options, 2, 320, game->menuSelection);
    }
}
