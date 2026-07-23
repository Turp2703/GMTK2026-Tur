#pragma once
#include "common.h"

enum class GameState {
    MENU,
    TYPE_SELECT,
    PLAYING,
    ROOM_COMPLETE,
    GAME_OVER,
    WIN
};

class Game {
public:
    Game();
    void Update(float dt);
    void Draw();

private:
    GameState state = GameState::MENU;

    int currentRoom = 1;
    int score = 0;

    // --- helpers -------------------------------------------------------
    static Difficulty RoomDifficulty(int roomNumber);

    void GoToTypeSelect(int roomNumber);

    void UpdateMenu();
    void UpdateTypeSelect();
    void UpdatePlaying(float dt);
    void UpdateRoomComplete(float dt);
    void UpdateGameOver();
    void UpdateWin();

    void DrawMenu();
    void DrawTypeSelect();
    void DrawPlaying();
    void DrawRoomComplete();
    void DrawGameOver();
    void DrawWin();
    void DrawHUD();
};
