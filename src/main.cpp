#include "raylib.h"
#include "game.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

void UpdateDrawFrame(void* arg) {
    Game* game = (Game*)arg;
    float dt = GetFrameTime();
    game->Update(dt);
    BeginDrawing();
        game->Draw();
    EndDrawing();
}

int main() {
    InitWindow((int)kScreenW, (int)kScreenH, "NOT-OBSOLETTE");
    InitAudioDevice();
    
    Game* game = new Game();
    
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(UpdateDrawFrame, game, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) UpdateDrawFrame(game);
#endif

    delete game;
    CloseAudioDevice();
    CloseWindow();
    return 0;
}