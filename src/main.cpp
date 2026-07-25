#include "raylib.h"
#include "game.h"

int main() {
    InitWindow((int)kScreenW, (int)kScreenH, "AI-Doomer");
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