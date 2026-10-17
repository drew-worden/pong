// Includes
#include "raylib.h"
#include "audio.h"
#include "game.h"
#include <math.h>

// Draws the fixed-resolution game texture scaled and letterboxed into the window
static void DrawScaled(RenderTexture2D target) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float scale = fminf(screenW / GAME_WIDTH, screenH / GAME_HEIGHT);

    Rectangle source = { 0, 0, GAME_WIDTH, -GAME_HEIGHT };
    Rectangle dest = {
        (screenW - GAME_WIDTH * scale) / 2.0f,
        (screenH - GAME_HEIGHT * scale) / 2.0f,
        GAME_WIDTH * scale,
        GAME_HEIGHT * scale,
    };
    DrawTexturePro(target.texture, source, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Pong");
    MaximizeWindow();
    InitAudioDevice();
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    Game game;
    GameInit(&game);
    AudioInit();

    while (!WindowShouldClose()) {
        GameUpdate(&game, GetFrameTime());

        BeginTextureMode(target);
        GameDraw(&game);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        DrawScaled(target);
        EndDrawing();
    }

    AudioClose();
    UnloadRenderTexture(target);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
