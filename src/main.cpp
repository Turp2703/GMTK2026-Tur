#include "raylib.h"
#include "game.h"

int main() {
    InitWindow(1280, 720, "AI-Doomer"); // TODO CHANGE WIDTH HEIGHT
    SetTargetFPS(60);

    Game game;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        game.Update(dt);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}