#pragma once
#include "common.h"
#include "level.h"

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
    ~Game();
    void Update(float dt);
    void Draw();

private:
    GameState state = GameState::MENU;

    int currentRoom = 1;
    static constexpr int kMaxRoom = 5;
    int score = 0;

    std::vector<PuzzleTypeInfo> typeCandidates;
    PuzzleTypeInfo chosenType{ "", "", Difficulty::EASY, nullptr };

    RoomLayout room;
    Player player;
    Enemy enemy;

    float timeRemaining = 60.0f;
    bool enemySpawned = false;
    int solvedCount = 0;
    float startCountdown = 0.0f;
    int nearbySlotIndex = -1;

    float messageTimer = 0.0f;
    std::string message;

    std::array<SpriteAnimation, Player::ANIM_COUNT> playerAnims;
    std::array<SpriteAnimation, Enemy::ANIM_COUNT> enemyAnims;
    SpriteAnimation sideDoorAnim;
    int entranceDoorFrame = 0;
    int exitDoorFrame = 7;
    bool entranceDoorClosing = false;
    bool exitDoorOpening = false;
    float doorTimer = 0.0f;
    SpriteAnimation enemyDoorAnim;
    int enemyDoorFrame = 0;
    bool enemyDoorAnimating = false;
    float enemyDoorTimer = 0.0f;
    Texture2D ladderTop{};
    Texture2D ladderMid{};
    Texture2D ladderBot{};
    SpriteAnimation sabotageAnim;

    // --- helpers -------------------------------------------------------
    static Difficulty RoomDifficulty(int roomNumber);

    void GoToTypeSelect(int roomNumber);
    void StartRoom(int roomNumber, const PuzzleTypeInfo& type);

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

    int FindNearbySlot(int playerFloor) const;
    void AnswerSlot(int slotIndex, int chosenChoice);
};
