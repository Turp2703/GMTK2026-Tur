#include "game.h"

// TODO MOVE
inline void DrawCenteredText(const std::string& text, Rectangle bounds, int fontSize, Color color) {
    int w = MeasureText(text.c_str(), fontSize);
    Vector2 pos{ bounds.x + bounds.width / 2 - w / 2.0f, bounds.y + bounds.height / 2 - fontSize / 2.0f };
    DrawText(text.c_str(), (int)pos.x, (int)pos.y, fontSize, color);
}

Game::Game() {}

// =============================================================================
// Setup
// =============================================================================
Difficulty Game::RoomDifficulty(int roomNumber) {
    if (roomNumber <= 2) return Difficulty::EASY;
    if (roomNumber <= 4) return Difficulty::MEDIUM;
    return Difficulty::HARD;
}

void Game::GoToTypeSelect(int roomNumber) {
}

// =============================================================================
// Update dispatch
// =============================================================================
void Game::Update(float dt) {
    switch (state) {
        case GameState::MENU: UpdateMenu(); break;
        case GameState::TYPE_SELECT: UpdateTypeSelect(); break;
        case GameState::PLAYING: UpdatePlaying(dt); break;
        case GameState::ROOM_COMPLETE: UpdateRoomComplete(dt); break;
        case GameState::GAME_OVER: UpdateGameOver(); break;
        case GameState::WIN: UpdateWin(); break;
    }
}

void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_ENTER)) {
        score = 0;
        GoToTypeSelect(1);
    }
}

void Game::UpdateTypeSelect() {
}

void Game::UpdatePlaying(float dt) {
}

void Game::UpdateRoomComplete(float dt) {
}

void Game::UpdateGameOver() {
}

void Game::UpdateWin() {
}

// =============================================================================
// Draw dispatch
// =============================================================================
void Game::Draw() {
    ClearBackground(Color{ 14, 12, 18, 255 });
    switch (state) {
        case GameState::MENU: DrawMenu(); break;
        case GameState::TYPE_SELECT: DrawTypeSelect(); break;
        case GameState::PLAYING: DrawPlaying(); break;
        case GameState::ROOM_COMPLETE: DrawPlaying(); DrawRoomComplete(); break;
        case GameState::GAME_OVER: DrawPlaying(); DrawGameOver(); break;
        case GameState::WIN: DrawWin(); break;
    }
}

void Game::DrawMenu() { // TODO WIDTH 1280
    DrawCenteredText("AI-DOOMER", Rectangle{ 0, 180, 1280, 80 }, 60, RAYWHITE);
    DrawCenteredText("You have a few minutes to prove you are better than AI.", Rectangle{ 0, 270, 1280, 30 }, 22, GRAY);
    DrawCenteredText("Or else... they will get you.", Rectangle{ 0, 300, 1280, 30 }, 22, Color{ 220, 80, 80, 255 });
    DrawCenteredText("AD move | W/S use elevator | 1/2/3 answer | Hold F to sabotage an elevator", Rectangle{ 0, 360, 1280, 30 }, 18, GRAY);
    DrawCenteredText("Press ENTER to start", Rectangle{ 0, 460, 1280, 40 }, 26, YELLOW);
}

void Game::DrawTypeSelect() {
}

void Game::DrawPlaying() {
}

void Game::DrawHUD() {
}

void Game::DrawRoomComplete() {
}

void Game::DrawGameOver() {
}

void Game::DrawWin() {
}