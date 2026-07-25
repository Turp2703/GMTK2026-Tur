#include "game.h"

Game::Game() {
    auto loadAnim = [](const char* path, int frames, float frameTime, bool loop) -> SpriteAnimation {
        SpriteAnimation a;
        a.texture = LoadTexture(path);
        a.frameCount = frames;
        a.frameTime = frameTime;
        a.loop = loop;
        return a;
    };
    playerAnims[Player::ANIM_SPAWN] = loadAnim("assets/player_spawn.png", 6, 0.25f, false);
    playerAnims[Player::ANIM_IDLE]  = loadAnim("assets/player_idle.png",  4, 0.15f, true);
    playerAnims[Player::ANIM_RUN]   = loadAnim("assets/player_run.png",   6, 0.08f, true);
    playerAnims[Player::ANIM_LADDER]= loadAnim("assets/player_ladder.png", 6, 0.1f,  true);
    playerAnims[Player::ANIM_FALL]  = loadAnim("assets/player_fall.png",  4, 0.2f, false);

    enemyAnims[Enemy::ANIM_SPAWN]  = loadAnim("assets/enemy_spawn.png",  6, 0.1f, false);
    enemyAnims[Enemy::ANIM_RUN]    = loadAnim("assets/enemy_run.png",    6, 0.1f, true);
    enemyAnims[Enemy::ANIM_LADDER] = loadAnim("assets/enemy_ladder.png", 6, 0.1f, true);
    enemyAnims[Enemy::ANIM_HIDDEN] = loadAnim("assets/enemy_hidden.png", 6, 0.1f, true);

    sideDoorAnim = loadAnim("assets/side_door.png", 5, 0.2f, false);
    enemyDoorAnim = loadAnim("assets/enemy_door.png", 10, 0.12f, false);
    ladderTop = LoadTexture("assets/ladder_top.png");
    ladderMid = LoadTexture("assets/ladder_middle.png");
    ladderBot = LoadTexture("assets/ladder_bottom.png");
    sabotageAnim = loadAnim("assets/sabotage.png", 4, 0.15f, true);
    floorTop = LoadTexture("assets/floor_top.png");
    floorTopLeft = LoadTexture("assets/floor_top_left.png");
    floorTopRight = LoadTexture("assets/floor_top_right.png");
    floorLeft = LoadTexture("assets/floor_left.png");
    floorRight = LoadTexture("assets/floor_right.png");
    floorLowest = LoadTexture("assets/floor_lowest.png");
    screenTex = LoadTexture("assets/screen.png");
}

Game::~Game() {
    for (auto& a : playerAnims) {
        if (a.texture.id != 0) UnloadTexture(a.texture);
    }
    for (auto& a : enemyAnims) {
        if (a.texture.id != 0) UnloadTexture(a.texture);
    }
    if (sideDoorAnim.texture.id != 0) UnloadTexture(sideDoorAnim.texture);
    if (enemyDoorAnim.texture.id != 0) UnloadTexture(enemyDoorAnim.texture);
    if (ladderTop.id != 0) UnloadTexture(ladderTop);
    if (ladderMid.id != 0) UnloadTexture(ladderMid);
    if (ladderBot.id != 0) UnloadTexture(ladderBot);
    if (sabotageAnim.texture.id != 0) UnloadTexture(sabotageAnim.texture);
    if (floorTop.id != 0) UnloadTexture(floorTop);
    if (floorTopLeft.id != 0) UnloadTexture(floorTopLeft);
    if (floorTopRight.id != 0) UnloadTexture(floorTopRight);
    if (floorLeft.id != 0) UnloadTexture(floorLeft);
    if (floorRight.id != 0) UnloadTexture(floorRight);
    if (floorLowest.id != 0) UnloadTexture(floorLowest);
    if (screenTex.id != 0) UnloadTexture(screenTex);
}

// =============================================================================
// Setup
// =============================================================================
Difficulty Game::RoomDifficulty(int roomNumber) {
    if (roomNumber <= 2) return Difficulty::EASY;
    if (roomNumber <= 4) return Difficulty::MEDIUM;
    return Difficulty::HARD;
}

void Game::GoToTypeSelect(int roomNumber) {
    Difficulty diff = RoomDifficulty(roomNumber);
    auto pool = PuzzleTypesForDifficulty(diff);
    std::shuffle(pool.begin(), pool.end(), RNG());

    typeCandidates.clear();
    int count = std::min((int)pool.size(), 3);
    for (int i = 0; i < count; i++) typeCandidates.push_back(*pool[i]);

    state = GameState::TYPE_SELECT;
}

void Game::StartRoom(int roomNumber, const PuzzleTypeInfo& type) {
    currentRoom = roomNumber; // TODO REMOVE
    chosenType = type;
    room = GenerateRoom(roomNumber, type);

    player = Player();
    player.pos = room.playerSpawn;
    player.controlsEnabled = false;
    playerAnims[Player::ANIM_SPAWN].Reset();

    enemy = Enemy();
    enemy.pos = room.enemySpawn;
    enemy.active = false;
    enemySpawned = false;

    entranceDoorFrame = sideDoorAnim.frameCount - 1;
    exitDoorFrame = 0;
    entranceDoorClosing = true;
    exitDoorOpening = false;
    enemyDoorFrame = 0;
    enemyDoorAnimating = false;
    enemyDoorTimer = 0.0f;

    timeRemaining = 60.0f;
    solvedCount = 0;
    startCountdown = 3.0f;
    nearbySlotIndex = -1;
    message.clear();
    messageTimer = 0.0f;

    state = GameState::PLAYING;
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
    if (IsKeyPressed(KEY_ONE) && typeCandidates.size() > 0) StartRoom(currentRoom, typeCandidates[0]);
    else if (IsKeyPressed(KEY_TWO) && typeCandidates.size() > 1) StartRoom(currentRoom, typeCandidates[1]);
    else if (IsKeyPressed(KEY_THREE) && typeCandidates.size() > 2) StartRoom(currentRoom, typeCandidates[2]);
}

int Game::FindNearbySlot(int playerFloor) const {
    Rectangle pb = player.Bounds();
    Rectangle interactZone{ pb.x - 25, pb.y - 25, pb.width + 50, pb.height + 50 };
    int best = -1;
    float bestDist = 1e9f;
    for (int i = 0; i < (int)room.puzzleSlots.size(); i++) {
        const auto& s = room.puzzleSlots[i];
        if (s.state != SlotState::UNSOLVED) continue;
        if (s.floor != playerFloor) continue;
        if (CheckCollisionRecs(interactZone, s.rect)) {
            float cx = s.rect.x + s.rect.width / 2 - (pb.x + pb.width / 2);
            float cy = s.rect.y + s.rect.height / 2 - (pb.y + pb.height / 2);
            float d = cx * cx + cy * cy;
            if (d < bestDist) { bestDist = d; best = i; }
        }
    }
    return best;
}

void Game::AnswerSlot(int slotIndex, int chosenChoice) {
    if (slotIndex < 0) return;
    auto& slot = room.puzzleSlots[slotIndex];
    if (slot.state != SlotState::UNSOLVED) return;

    if (chosenChoice == slot.puzzle->GetCorrectIndex()) {
        slot.state = SlotState::SOLVED;
        int pts = slot.puzzle->GetPoints();
        score += pts;
        if (!slot.isBonus) solvedCount++;
        message = TextFormat(slot.isBonus ? "Bonus solved! +%d pts" : "Correct! +%d pts", pts);
        messageTimer = 1.2f;
    } else {
        slot.state = SlotState::WRONG_COOLDOWN;
        slot.cooldownTimer = PuzzleSlot::kCooldown;
        timeRemaining = std::max(0.0f, timeRemaining - 5.0f);
        message = "Wrong answer (-5s)";
        messageTimer = 1.0f;
    }
}

void Game::UpdatePlaying(float dt) {
    // Elevators always tick (so they can reactivate) even during the intro countdown.
    for (auto& e : room.elevators) e.Update(dt);

    if (startCountdown > 0.0f) {
        startCountdown -= dt;
        player.controlsEnabled = false;
    } else {
        player.controlsEnabled = true;
    }

    player.Update(dt, room.platforms, room.elevators, kScreenW, kScreenH, room.floorY[0]);
    if (player.grounded) playerAnims[Player::ANIM_FALL].Reset();
    for (auto& a : playerAnims) a.Update(dt);
    sabotageAnim.Update(dt);

    // Entrance door: close during spawn countdown
    if (entranceDoorClosing && entranceDoorFrame > 0) {
        doorTimer += dt;
        if (doorTimer >= sideDoorAnim.frameTime) {
            doorTimer -= sideDoorAnim.frameTime;
            entranceDoorFrame--;
        }
    }
    if (startCountdown <= 0.0f) entranceDoorClosing = false;

    // Exit door: open when puzzles are solved
    if (solvedCount >= room.requiredSolves && !exitDoorOpening) {
        exitDoorOpening = true;
        doorTimer = 0.0f;
    }
    if (exitDoorOpening && exitDoorFrame < sideDoorAnim.frameCount - 1) {
        doorTimer += dt;
        if (doorTimer >= sideDoorAnim.frameTime) {
            doorTimer -= sideDoorAnim.frameTime;
            exitDoorFrame++;
        }
    }

    if (startCountdown > 0.0f) return; // nothing else happens until "GO"

    if (messageTimer > 0) messageTimer -= dt;

    int playerFloor = room.FloorForY(player.pos.y + player.size.y);

    // Sabotage buttons - press F while standing near one.
    Rectangle pb = player.Bounds();
    Rectangle interactZone{ pb.x - 20, pb.y - 20, pb.width + 40, pb.height + 40 };
    for (auto& btn : room.sabotageButtons) {
        auto& elev = room.elevators[btn.elevatorIndex];
        bool near = CheckCollisionRecs(interactZone, btn.rect) && elev.floorLow == playerFloor;
        if (near && !elev.used && IsKeyPressed(KEY_F)) {
            elev.active = false;
            elev.used = true;
            elev.reactivateTimer = 5.0f;
            message = "Elevator sabotaged!";
            messageTimer = 1.2f;
        }
    }

    // Puzzle slots: cooldown ticking + regeneration.
    for (auto& slot : room.puzzleSlots) {
        if (slot.state == SlotState::WRONG_COOLDOWN) {
            slot.cooldownTimer -= dt;
            if (slot.cooldownTimer <= 0.0f) {
                slot.puzzle = slot.regenerate();
                slot.state = SlotState::UNSOLVED;
            }
        }
    }

    nearbySlotIndex = FindNearbySlot(playerFloor);
    if (nearbySlotIndex >= 0) {
        if (IsKeyPressed(KEY_ONE)) AnswerSlot(nearbySlotIndex, 0);
        else if (IsKeyPressed(KEY_TWO)) AnswerSlot(nearbySlotIndex, 1);
        else if (IsKeyPressed(KEY_THREE)) AnswerSlot(nearbySlotIndex, 2);
    }

    if (!enemySpawned) {
        timeRemaining -= dt;
        float doorAnimDuration = enemyDoorAnim.frameCount * enemyDoorAnim.frameTime;

        // Start door animation when time is about to run out
        if (!enemyDoorAnimating && timeRemaining <= doorAnimDuration) {
            enemyDoorAnimating = true;
            enemyDoorTimer = 0.0f;
        }

        // Advance door animation
        if (enemyDoorAnimating && enemyDoorFrame < enemyDoorAnim.frameCount - 1) {
            enemyDoorTimer += dt;
            if (enemyDoorTimer >= enemyDoorAnim.frameTime) {
                enemyDoorTimer -= enemyDoorAnim.frameTime;
                enemyDoorFrame++;
            }
        }

        // Spawn enemy once door animation finishes
        if (enemyDoorAnimating && enemyDoorFrame >= enemyDoorAnim.frameCount - 1) {
            timeRemaining = 0.0f;
            enemy.Spawn(room.enemySpawn);
            enemySpawned = true;
            enemyAnims[Enemy::ANIM_SPAWN].Reset();
        }
    } else {
        for (auto& a : enemyAnims) a.Update(dt);
        if (enemy.spawning && enemyAnims[Enemy::ANIM_SPAWN].finished) {
            enemy.spawning = false;
        }
        enemy.Update(dt, player, room.platforms, room.elevators, kScreenH);
        enemy.pos.x = Clamp(enemy.pos.x, 16.0f, kScreenW - enemy.size.x - 16.0f);
        if (enemy.CatchesPlayer(player)) {
            state = GameState::GAME_OVER;
            return;
        }
    }

    if (solvedCount >= room.requiredSolves && CheckCollisionRecs(player.Bounds(), room.exitDoor.rect)) {
        if (currentRoom >= kMaxRoom) {
            state = GameState::WIN;
        } else {
            state = GameState::ROOM_COMPLETE;
            messageTimer = 1.6f;
        }
    }
}

void Game::UpdateRoomComplete(float dt) {
    messageTimer -= dt;
    if (messageTimer <= 0) {
        currentRoom++;
        GoToTypeSelect(currentRoom);
    }
}

void Game::UpdateGameOver() {
    if (IsKeyPressed(KEY_ENTER)) {
        currentRoom = 1;
        score = 0;
        state = GameState::MENU;
    }
}

void Game::UpdateWin() {
    if (IsKeyPressed(KEY_ENTER)) {
        currentRoom = 1;
        score = 0;
        state = GameState::MENU;
    }
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
    DrawCenteredText("AI-DOOMER", Rectangle{ 0, 180, kScreenW, 80 }, 60, RAYWHITE);
    DrawCenteredText("You have a few minutes to prove you are better than AI.", Rectangle{ 0, 270, kScreenW, 30 }, 22, GRAY);
    DrawCenteredText("Or else... they will get you.", Rectangle{ 0, 300, kScreenW, 30 }, 22, Color{ 220, 80, 80, 255 });
    DrawCenteredText("AD move | W/S use elevator | 1/2/3 answer | F to sabotage an elevator", Rectangle{ 0, 360, kScreenW, 30 }, 18, GRAY);
    DrawCenteredText("Press ENTER to start", Rectangle{ 0, 460, kScreenW, 40 }, 26, YELLOW);
}

void Game::DrawTypeSelect() {
    DrawCenteredText(TextFormat("SECTOR %d - Choose your test type", currentRoom), Rectangle{ 0, 100, kScreenW, 50 }, 34, RAYWHITE);
    DrawCenteredText(TextFormat("Difficulty: %s", DifficultyName(RoomDifficulty(currentRoom))), Rectangle{ 0, 150, kScreenW, 30 }, 20, DifficultyColor(RoomDifficulty(currentRoom)));

    float cardW = 340, cardH = 260, gap = 40;
    float totalW = cardW * typeCandidates.size() + gap * (typeCandidates.size() > 0 ? typeCandidates.size() - 1 : 0);
    float startX = kScreenW / 2 - totalW / 2;
    float y = 250;
    for (int i = 0; i < (int)typeCandidates.size(); i++) {
        const auto& opt = typeCandidates[i];
        Rectangle card{ startX + i * (cardW + gap), y, cardW, cardH };
        DrawRectangleRec(card, Color{ 30, 30, 42, 255 });
        DrawRectangleLinesEx(card, 3, DifficultyColor(opt.difficulty));
        DrawCenteredText(TextFormat("[%d]", i + 1), Rectangle{ card.x, card.y + 16, card.width, 26 }, 20, GRAY);
        DrawCenteredText(opt.name, Rectangle{ card.x, card.y + 60, card.width, 40 }, 28, RAYWHITE);
        DrawCenteredText(opt.description, Rectangle{ card.x + 10, card.y + 130, card.width - 20, 80 }, 16, RAYWHITE);
    }
    DrawCenteredText("Press 1, 2 or 3", Rectangle{ 0, y + cardH + 40, kScreenW, 30 }, 22, YELLOW);
}

void Game::DrawPlaying() {
    int playerFloor = room.FloorForY(player.pos.y + player.size.y);
    int enemyFloor = enemy.active ? room.FloorForY(enemy.pos.y + enemy.size.y) : -1;

    auto floorAlpha = [&](int floor) -> unsigned char { return floor == playerFloor ? 255 : 70; };

    // Platforms
    for (auto& p : room.platforms) {
        unsigned char a = floorAlpha(p.floor);
        Color tint{ 255, 255, 255, a };
        float px = p.rect.x, py = p.rect.y, pw = p.rect.width;

        Texture2D tileTex = (p.floor == 0) ? floorLowest : floorTop;
        Texture2D leftTex = (p.floor == 0) ? floorLowest : floorTopLeft;
        Texture2D rightTex = (p.floor == 0) ? floorLowest : floorTopRight;

        DrawTexturePro(leftTex, { 0, 0, 32, 32 }, { px, py, 32, 32 }, {0,0}, 0.0f, tint);
        float tileX = 32.0f;
        while (tileX < pw - 32.0f) {
            float w = std::min(32.0f, pw - 32.0f - tileX);
            DrawTexturePro(tileTex, { 0, 0, w, 32 }, { px + tileX, py, w, 32 }, {0,0}, 0.0f, tint);
            tileX += 32.0f;
        }
        if (pw >= 64.0f) {
            DrawTexturePro(rightTex, { 0, 0, 32, 32 }, { px + pw - 32, py, 32, 32 }, {0,0}, 0.0f, tint);
        }
    }

    // Ceiling (y=32, below HUD)
    {
        float cy = 32.0f;
        Color tint{ 255, 255, 255, 255 };
        DrawTexturePro(floorTopLeft, { 0, 0, 32, 32 }, { 0, cy, 32, 32 }, {0,0}, 0.0f, tint);
        float tileX = 32.0f;
        while (tileX < kScreenW - 32.0f) {
            float w = std::min(32.0f, kScreenW - 32.0f - tileX);
            DrawTexturePro(floorTop, { 0, 0, w, 32 }, { tileX, cy, w, 32 }, {0,0}, 0.0f, tint);
            tileX += 32.0f;
        }
        DrawTexturePro(floorTopRight, { 0, 0, 32, 32 }, { kScreenW - 32, cy, 32, 32 }, {0,0}, 0.0f, tint);
    }

    // Walls
    {
        Color tint{ 255, 255, 255, 255 };
        float wallBottom = room.floorY[0];
        Rectangle doorL = room.entranceDoor.rect;
        Rectangle doorR = room.exitDoor.rect;

        // Left wall
        float wy = 32.0f;
        while (wy < wallBottom) {
            if (wy + 32.0f > doorL.y && wy < doorL.y + doorL.height) {
                wy = doorL.y + doorL.height;
                continue;
            }
            DrawTexturePro(floorLeft, { 0, 0, 32, 32 }, { 0, wy, 32, 32 }, {0,0}, 0.0f, tint);
            wy += 32.0f;
        }

        // Right wall
        wy = 32.0f;
        while (wy < wallBottom) {
            if (wy + 32.0f > doorR.y && wy < doorR.y + doorR.height) {
                wy = doorR.y + doorR.height;
                continue;
            }
            DrawTexturePro(floorRight, { 0, 0, 32, 32 }, { kScreenW - 32, wy, 32, 32 }, {0,0}, 0.0f, tint);
            wy += 32.0f;
        }
    }

    // Elevators (ladders)
    for (int i = 0; i < (int)room.elevators.size(); i++) {
        auto& e = room.elevators[i];
        int loF = e.floorLow, hiF = e.floorHigh;
        unsigned char a = (loF == playerFloor || hiF == playerFloor) ? 255 : 70;
        Color tint = e.active ? Color{ 255, 255, 255, a } : Color{ 180, 80, 80, a };

        float ladderW = 24.0f;
        float drawX = e.rect.x + e.rect.width / 2.0f - ladderW / 2.0f;

        bool isTopmostInCol = true;
        for (int j = 0; j < (int)room.elevators.size(); j++) {
            if (j == i) continue;
            auto& o = room.elevators[j];
            if ((int)o.rect.x == (int)e.rect.x && o.rect.y < e.rect.y) {
                isTopmostInCol = false;
                break;
            }
        }

        float visBottom = room.floorY[loF];

        float topY = isTopmostInCol ? (e.rect.y - 32.0f) : e.rect.y;
        float tileY = isTopmostInCol ? e.rect.y : (e.rect.y + 32.0f);

        Texture2D topTex = isTopmostInCol ? ladderTop : ladderBot;
        DrawTexturePro(topTex, { 0, 0, 32, 32 }, { drawX, topY, 32, 32 }, Vector2{0,0}, 0.0f, tint);
        while (tileY < visBottom) {
            float h = std::min(32.0f, visBottom - tileY);
            DrawTexturePro(ladderBot, { 0, 0, 32, h }, { drawX, tileY, 32, h }, Vector2{0,0}, 0.0f, tint);
            tileY += 32.0f;
        }
        if (loF > 0) {
            DrawTexturePro(ladderMid, { 0, 0, 32, 32 }, { drawX, visBottom, 32, 32 }, Vector2{0,0}, 0.0f, tint);
        }
    }

    // Sabotage buttons
    if (sabotageAnim.texture.id != 0) {
        float btnSpriteW = (float)sabotageAnim.texture.width / sabotageAnim.frameCount;
        float btnSpriteH = (float)sabotageAnim.texture.height;
        for (auto& btn : room.sabotageButtons) {
            auto& elev = room.elevators[btn.elevatorIndex];
            unsigned char a = (elev.floorLow == playerFloor) ? 255 : 70;
            Color tint = elev.used ? Color{ 120, 120, 120, a } : WHITE;
            tint.a = a;
            Rectangle src = sabotageAnim.SourceRect(btn.flipped);
            DrawTexturePro(sabotageAnim.texture, src,
                { btn.rect.x, btn.rect.y, btnSpriteW, btnSpriteH },
                Vector2{0,0}, 0.0f, tint);
        }
    } else {
        for (auto& btn : room.sabotageButtons) {
            auto& elev = room.elevators[btn.elevatorIndex];
            unsigned char a = (elev.floorLow == playerFloor) ? 255 : 70;
            Color c = elev.used ? Color{ 80, 80, 80, a } : Color{ 200, 60, 60, a };
            DrawRectangleRec(btn.rect, c);
            DrawRectangleLinesEx(btn.rect, 1, Color{ 20, 20, 20, a });
        }
    }

    // Doors
    if (sideDoorAnim.texture.id != 0) {
        float spriteW = (float)sideDoorAnim.texture.width / sideDoorAnim.frameCount;
        float spriteH = (float)sideDoorAnim.texture.height;
        float gapAbove = fmodf(room.entranceDoor.rect.y - 32.0f, 32.0f);

        // Entrance door (flipped horizontally)
        Rectangle entranceSrc = { (entranceDoorFrame + 1) * spriteW, 0, -spriteW, spriteH };
        DrawTexturePro(sideDoorAnim.texture, entranceSrc,
            { room.entranceDoor.rect.x, room.entranceDoor.rect.y - gapAbove, room.entranceDoor.rect.width, room.entranceDoor.rect.height + gapAbove },
            Vector2{0,0}, 0.0f, WHITE);

        // Exit door
        Rectangle exitSrc = { exitDoorFrame * spriteW, 0, spriteW, spriteH };
        DrawTexturePro(sideDoorAnim.texture, exitSrc,
            { room.exitDoor.rect.x, room.exitDoor.rect.y - gapAbove, room.exitDoor.rect.width, room.exitDoor.rect.height + gapAbove },
            Vector2{0,0}, 0.0f, WHITE);
    } else {
        DrawRectangleRec(room.entranceDoor.rect, Color{ 60, 100, 60, 255 });
        DrawCenteredText("A", room.entranceDoor.rect, 20, RAYWHITE);

        bool exitReady = solvedCount >= room.requiredSolves;
        DrawRectangleRec(room.exitDoor.rect, exitReady ? Color{ 60, 160, 90, 255 } : Color{ 90, 60, 60, 255 });
        DrawCenteredText("B", room.exitDoor.rect, 20, RAYWHITE);
    }

    // Enemy spawn door (top floor, column 3)
    if (enemyDoorAnim.texture.id != 0) {
        int doorFloor = room.FloorForY(room.enemyDoorRect.y + room.enemyDoorRect.height);
        unsigned char doorA = floorAlpha(doorFloor);
        float spriteW = (float)enemyDoorAnim.texture.width / enemyDoorAnim.frameCount;
        float spriteH = (float)enemyDoorAnim.texture.height;
        Rectangle src = { enemyDoorFrame * spriteW, 0, spriteW, spriteH };
        DrawTexturePro(enemyDoorAnim.texture, src,
            { room.enemyDoorRect.x, room.enemyDoorRect.y, spriteW, spriteH },
            Vector2{0,0}, 0.0f, Color{ 255, 255, 255, doorA });
    }

    // Puzzle slots
    for (int i = 0; i < (int)room.puzzleSlots.size(); i++) {
        auto& s = room.puzzleSlots[i];
        unsigned char a = (s.floor == playerFloor) ? 255 : 70;

        Color tint{ 255, 255, 255, a };
        if (s.state == SlotState::SOLVED) tint = Color{ 30, 90, 40, a };
        else if (s.state == SlotState::WRONG_COOLDOWN) tint = Color{ 130, 40, 40, a };
        else if (s.state == SlotState::BROKEN) tint = Color{ 35, 35, 35, a };

        float sw = (float)screenTex.width, sh = (float)screenTex.height;
        float dx = s.rect.x + s.rect.width / 2 - sw / 2;
        float dy = s.rect.y + s.rect.height / 2 - sh / 2;
        DrawTexturePro(screenTex, { 0, 0, sw, sh }, { dx, dy, sw, sh }, {0,0}, 0.0f, tint);
        float inset = 10.0f;

        Rectangle textArea{ dx + inset, dy + inset, sw - inset * 2, sh - inset * 2 };

        if (s.floor != playerFloor) {
            DrawCenteredText("- - -", textArea, 20, Color{ 120, 120, 130, a });
        } else if (s.state == SlotState::BROKEN) {
            DrawCenteredText("OUT OF\nORDER", textArea, 14, Color{ 150, 150, 150, 255 });
        } else if (s.state == SlotState::SOLVED) {
            DrawCenteredText("OK", textArea, 26, Color{ 150, 255, 150, 255 });
        } else if (s.state == SlotState::WRONG_COOLDOWN) {
            DrawCenteredText("X", textArea, 30, Color{ 255, 150, 150, 255 });
            float pct = 1.0f - (s.cooldownTimer / PuzzleSlot::kCooldown);
            DrawRectangle((int)textArea.x, (int)textArea.y - 8, (int)(textArea.width * pct), 4, Color{ 255, 200, 60, 255 });
        } else {
            Rectangle inner{ textArea.x + 4, textArea.y + 4, textArea.width - 8, textArea.height - 8 };
            s.puzzle->RenderQuestion(inner);
        }

        if (s.isBonus && s.floor == playerFloor && s.state != SlotState::BROKEN) {
            DrawCenteredText("BONUS", Rectangle{ textArea.x, dy - 20, textArea.width, 16 }, 12, Color{ 230, 190, 60, 255 });
        }
    }

    // Answer choices for the nearby active slot.
    if (nearbySlotIndex >= 0 && nearbySlotIndex < (int)room.puzzleSlots.size()) {
        auto& s = room.puzzleSlots[nearbySlotIndex];
        if (s.state == SlotState::UNSOLVED) {
            auto choices = s.puzzle->GetChoices();
            float btnW = 130, btnH = 34, gap = 8;
            float totalW = btnW * 3 + gap * 2;
            float x = s.rect.x + s.rect.width / 2 - totalW / 2;
            float y = s.rect.y - 34 - btnH;
            for (int c = 0; c < 3; c++) {
                Rectangle btn{ x + c * (btnW + gap), y, btnW, btnH };
                DrawRectangleRec(btn, Color{ 50, 54, 70, 255 });
                DrawRectangleLinesEx(btn, 2, Color{ 150, 170, 220, 255 });
                DrawCenteredText(TextFormat("[%d] %s", c + 1, choices[c].c_str()), btn, 14, RAYWHITE);
            }
        }
    }

    if (enemy.active) enemy.Render(enemyAnims, enemyFloor != playerFloor, floorAlpha(enemyFloor));
    player.Render(playerAnims);

    DrawHUD();

    if (startCountdown > 0.0f) {
        DrawRectangle(0, 0, (int)kScreenW, (int)kScreenH, Color{ 0, 0, 0, 120 });
        int n = (int)ceilf(startCountdown);
        DrawCenteredText(n > 0 ? TextFormat("%d", n) : "GO!", Rectangle{ 0, kScreenH / 2 - 60, kScreenW, 100 }, 90, RAYWHITE);
    }

    if (messageTimer > 0 && !message.empty() && state == GameState::PLAYING) {
        DrawCenteredText(message, Rectangle{ 0, 90, kScreenW, 30 }, 22, YELLOW);
    }
}

void Game::DrawHUD() {
    DrawRectangle(0, 0, (int)kScreenW, 50, Color{ 8, 8, 12, 220 });
    DrawText(TextFormat("Sector: %d", currentRoom), 16, 14, 20, RAYWHITE);
    DrawText(TextFormat("Score: %d", score), 170, 14, 20, RAYWHITE);
    DrawText(TextFormat("Tests: %d/%d", solvedCount, room.requiredSolves), 340, 14, 20, RAYWHITE);

    Color timeColor = enemySpawned ? RED : (timeRemaining < 15 ? ORANGE : RAYWHITE);
    DrawText(TextFormat(enemySpawned ? "Time: 0 - CONTAINMENT UNIT DISPATCHED" : "Time: %d", (int)ceilf(timeRemaining)), 520, 14, 20, timeColor);
    DrawText(chosenType.name.c_str(), (int)kScreenW - 220, 14, 20, DifficultyColor(chosenType.difficulty));
}

void Game::DrawRoomComplete() {
    DrawRectangle(0, 0, (int)kScreenW, (int)kScreenH, Color{ 0, 0, 0, 150 });
    DrawCenteredText("SECTOR CLEARED", Rectangle{ 0, kScreenH / 2 - 40, kScreenW, 60 }, 48, Color{ 120, 255, 140, 255 });
}

void Game::DrawGameOver() {
    DrawRectangle(0, 0, (int)kScreenW, (int)kScreenH, Color{ 0, 0, 0, 180 });
    DrawCenteredText("EVALUATION FAILED", Rectangle{ 0, kScreenH / 2 - 100, kScreenW, 60 }, 48, Color{ 255, 90, 90, 255 });
    DrawCenteredText("Containment Unit Dispatched", Rectangle{ 0, kScreenH / 2 - 45, kScreenW, 30 }, 20, Color{ 220, 150, 150, 255 });
    DrawCenteredText(TextFormat("Final score: %d", score), Rectangle{ 0, kScreenH / 2 - 5, kScreenW, 40 }, 26, RAYWHITE);
    DrawCenteredText("Press ENTER to return to the menu", Rectangle{ 0, kScreenH / 2 + 40, kScreenW, 40 }, 22, YELLOW);
}

void Game::DrawWin() {
    DrawCenteredText("HUMANITY: NOT OBSOLETE", Rectangle{ 0, kScreenH / 2 - 110, kScreenW, 60 }, 46, Color{ 255, 215, 90, 255 });
    DrawCenteredText("You proved you are better than AI.", Rectangle{ 0, kScreenH / 2 - 50, kScreenW, 30 }, 22, RAYWHITE);
    DrawCenteredText(TextFormat("Final score: %d", score), Rectangle{ 0, kScreenH / 2 - 10, kScreenW, 40 }, 28, RAYWHITE);
    DrawCenteredText("Press ENTER to return to the menu", Rectangle{ 0, kScreenH / 2 + 40, kScreenW, 40 }, 22, YELLOW);
}