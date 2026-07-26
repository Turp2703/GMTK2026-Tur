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
    static constexpr bool kDebug = true; // TODO SET FALSE
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
    Texture2D screenTex{};
    SpriteAnimation doorParticleAnim{};
    bool doorParticleActive = false;
    Texture2D overlayTex[3]{};
    int overlayStage = 0;
    float overlayBlend = 0.0f;

    Texture2D iconEnemy{};
    Texture2D iconExit{};
    float exitIconTimer = 0.0f;

    static constexpr int kNoiseFrames = 4;
    Texture2D noiseTex[kNoiseFrames]{};
    int noiseFrame = 0;
    float noiseTimer = 0.0f;

    struct Particle {
        Vector2 pos;
        Vector2 vel;
        float lifetime;
        float age = 0.0f;
        int frame = 0;
        float frameTimer = 0.0f;
        bool flipX = false;
    };
    static constexpr int kMaxParticles = 2;
    std::vector<Particle> particles;
    float particleSpawnTimer = 0.0f;
    Texture2D runParticleTex{};

    Texture2D floorTop{};
    Texture2D floorTopLeft{};
    Texture2D floorTopRight{};
    Texture2D floorLeft{};
    Texture2D floorRight{};
    Texture2D floorLowest{};

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
