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
    overlayTex[0] = LoadTexture("assets/overlay_1.png");
    overlayTex[1] = LoadTexture("assets/overlay_2.png");
    overlayTex[2] = LoadTexture("assets/overlay_3.png");
    iconEnemy = LoadTexture("assets/icon_enemy.png");
    iconExit = LoadTexture("assets/icon_exit.png");

    int nw = (int)screenTex.width, nh = (int)screenTex.height;
    for (int i = 0; i < kNoiseFrames; i++) {
        Image img = GenImageWhiteNoise(nw, nh, 0.45f);
        noiseTex[i] = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    runParticleTex = LoadTexture("assets/player_run_particles.png");
    doorParticleAnim = loadAnim("assets/side_door_particles.png", 6, 0.1f, false);
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
    for (int i = 0; i < 3; i++) {
        if (overlayTex[i].id != 0) UnloadTexture(overlayTex[i]);
    }
    if (iconEnemy.id != 0) UnloadTexture(iconEnemy);
    if (iconExit.id != 0) UnloadTexture(iconExit);
    for (int i = 0; i < kNoiseFrames; i++) {
        if (noiseTex[i].id != 0) UnloadTexture(noiseTex[i]);
    }
    if (runParticleTex.id != 0) UnloadTexture(runParticleTex);
    if (doorParticleAnim.texture.id != 0) UnloadTexture(doorParticleAnim.texture);
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
    examplePuzzles.clear();
    exampleChoices.clear();
    int count = std::min((int)pool.size(), 3);
    for (int i = 0; i < count; i++) {
        typeCandidates.push_back(*pool[i]);
        auto ex = pool[i]->factory();
        ex->Generate();
        exampleChoices.push_back(ex->GetChoices());
        examplePuzzles.push_back(std::move(ex));
    }

    screenTimer = 0.0f;
    state = GameState::TYPE_SELECT;
}

void Game::StartRoom(int roomNumber, const PuzzleTypeInfo& type) {
    currentRoom = roomNumber;
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
    overlayStage = 0;
    overlayBlend = 0.0f;
    exitIconTimer = 0.0f;
    particles.clear();
    particleSpawnTimer = 0.0f;
    doorParticleActive = false;

    state = GameState::PLAYING;
}

// =============================================================================
// Update dispatch
// =============================================================================
void Game::Update(float dt) {
    // Fade transition
    if (fadingOut) {
        fadeAlpha = std::min(1.0f, fadeAlpha + fadeSpeed * dt);
        if (fadeAlpha >= 1.0f && fadeHasAction) {
            fadeHasAction = false;
            screenTimer = 0.0f;
            if (fadeActionType == 0) {
                // Generic: reset and go to state
                score = 0;
                currentRoom = 1;
                state = fadeNextState;
            } else if (fadeActionType == 1) {
                // Menu → type select
                score = 0;
                GoToTypeSelect(1);
            }
            fadingOut = false;
        }
    } else if (fadeAlpha > 0.0f) {
        fadeAlpha = std::max(0.0f, fadeAlpha - fadeSpeed * dt);
    }

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
    screenTimer += GetFrameTime();
    if (IsKeyPressed(KEY_ENTER) && !fadingOut) {
        fadingOut = true;
        fadeSpeed = 2.5f;
        fadeNextState = GameState::TYPE_SELECT;
        fadeHasAction = true;
        fadeActionType = 1;
    }
}

void Game::UpdateTypeSelect() {
    screenTimer += GetFrameTime();
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
        else solvedCount += 2;
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
    if (doorParticleActive) {
        doorParticleAnim.Update(dt);
        if (doorParticleAnim.finished) doorParticleActive = false;
    }

    noiseTimer += dt;
    if (noiseTimer >= 0.1f) {
        noiseTimer -= 0.1f;
        noiseFrame = (noiseFrame + 1) % kNoiseFrames;
    }

    // Run particles
    if (player.grounded && player.wasMoving && !player.climbing) {
        particleSpawnTimer += dt;
        if (particleSpawnTimer >= 0.24f && (int)particles.size() < kMaxParticles) {
            particleSpawnTimer = 0.0f;
            Particle p;
            float behindX = player.facingRight ? -8 : 8;
            p.pos = { player.pos.x + player.size.x / 2 + behindX, player.pos.y + player.size.y - 24 };
            p.vel = { player.facingRight ? RandFloat(-40, -15) : RandFloat(15, 40), RandFloat(-50, -25) };
            p.lifetime = 0.35f;
            p.age = 0.0f;
            p.frame = 0;
            p.frameTimer = 0.0f;
            p.flipX = !player.facingRight;
            particles.push_back(p);
        }
    } else {
        particleSpawnTimer = 0.12f;
    }
    float frameDur = runParticleTex.width > 0 ? 0.4f / 6 : 0.1f;
    for (auto& p : particles) {
        p.age += dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.frameTimer += dt;
        if (p.frameTimer >= frameDur) {
            p.frameTimer -= frameDur;
            if (p.frame < 5) p.frame++;
        }
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.age >= p.lifetime; }),
        particles.end());

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
        doorParticleActive = true;
        doorParticleAnim.Reset();
    }
    if (exitDoorOpening && exitDoorFrame < sideDoorAnim.frameCount - 1) {
        doorTimer += dt;
        if (doorTimer >= sideDoorAnim.frameTime) {
            doorTimer -= sideDoorAnim.frameTime;
            exitDoorFrame++;
        }
    }
    if (exitDoorOpening) exitIconTimer += dt;

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
            message = "Ladder sabotaged!";
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

    if (kDebug) {
        if (IsKeyPressed(KEY_I)) {
            for (auto& slot : room.puzzleSlots) {
                if (slot.state == SlotState::UNSOLVED) {
                    slot.state = SlotState::SOLVED;
                    if (slot.isBonus) solvedCount += 2;
                    else solvedCount++;
                }
            }
            message = "All puzzles solved (debug)";
            messageTimer = 1.0f;
        }
        if (IsKeyPressed(KEY_O)) {
            timeRemaining = std::max(0.0f, timeRemaining - 5.0f);
            message = "-5s (debug)";
            messageTimer = 1.0f;
        }
        if (IsKeyPressed(KEY_P)) {
            state = GameState::MENU;
            return;
        }
    }

    if (!enemySpawned) {
        timeRemaining -= dt;

        // Overlay transitions
        if (overlayStage == 0 && timeRemaining <= 30.0f) {
            overlayStage = 1;
            overlayBlend = 0.0f;
        }
        if (overlayStage == 1 && timeRemaining <= 10.0f) {
            overlayStage = 2;
            overlayBlend = 0.0f;
        }
        if (overlayBlend < 1.0f && overlayStage > 0) {
            overlayBlend = std::min(1.0f, overlayBlend + dt * 0.5f);
        }

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
            screenTimer = 0.0f;
            state = GameState::GAME_OVER;
            return;
        }
    }

    if (solvedCount >= room.requiredSolves && CheckCollisionRecs(player.Bounds(), room.exitDoor.rect)) {
        if (currentRoom >= kMaxRoom) {
            screenTimer = 0.0f;
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
    screenTimer += GetFrameTime();
    if (IsKeyPressed(KEY_ENTER) && !fadingOut) {
        fadingOut = true;
        fadeSpeed = 2.5f;
        fadeNextState = GameState::MENU;
        fadeHasAction = true;
        fadeActionType = 0;
    }
}

void Game::UpdateWin() {
    screenTimer += GetFrameTime();
    if (IsKeyPressed(KEY_ENTER) && !fadingOut) {
        fadingOut = true;
        fadeSpeed = 2.5f;
        fadeNextState = GameState::MENU;
        fadeHasAction = true;
        fadeActionType = 0;
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
    if (fadeAlpha > 0.0f) {
        DrawRectangle(0, 0, (int)kScreenW, (int)kScreenH, Color{ 0, 0, 0, (unsigned char)(fadeAlpha * 255) });
    }
}

void Game::DrawGradientBg(Color top, Color bottom) {
    for (int y = 0; y < (int)kScreenH; y++) {
        float t = (float)y / kScreenH;
        Color c{
            (unsigned char)(top.r + (bottom.r - top.r) * t),
            (unsigned char)(top.g + (bottom.g - top.g) * t),
            (unsigned char)(top.b + (bottom.b - top.b) * t),
            255
        };
        DrawRectangle(0, y, (int)kScreenW, 1, c);
    }
    float scanY = fmodf(screenTimer * 80.0f, kScreenH + 40) - 20;
    DrawRectangle(0, (int)scanY, (int)kScreenW, 2, Color{ 255, 255, 255, 15 });
}

void Game::DrawMenu() {
    DrawGradientBg(Color{ 8, 6, 14 }, Color{ 18, 14, 30 });

    float fadeIn = std::min(screenTimer / 1.0f, 1.0f);
    float pulse = 0.85f + 0.15f * sinf(screenTimer * 2.5f);
    float titleY = 140 + (1.0f - fadeIn) * 30;

    DrawCenteredText("AI-DOOMER", Rectangle{ 0, titleY, kScreenW, 80 }, 64,
        Color{ (unsigned char)(255 * pulse), (unsigned char)(220 * pulse), (unsigned char)(80 * pulse), (unsigned char)(255 * fadeIn) });

    float subFade = std::min(std::max((screenTimer - 0.4f) / 0.8f, 0.0f), 1.0f);
    DrawCenteredText("You have a few minutes to prove you are better than AI.", Rectangle{ 0, 260, kScreenW, 30 }, 22,
        Color{ 160, 165, 180, (unsigned char)(255 * subFade) });

    float warnFade = std::min(std::max((screenTimer - 0.8f) / 0.8f, 0.0f), 1.0f);
    float warnPulse = 0.8f + 0.2f * sinf(screenTimer * 3.0f);
    DrawCenteredText("Or else... they will get you.", Rectangle{ 0, 295, kScreenW, 30 }, 22,
        Color{ (unsigned char)(220 * warnPulse), 70, 70, (unsigned char)(255 * warnFade) });

    float ctrlFade = std::min(std::max((screenTimer - 1.2f) / 0.6f, 0.0f), 1.0f);
    DrawCenteredText("A/D move  |  W/S climb ladders  |  1/2/3 answer  |  F sabotage", Rectangle{ 0, 370, kScreenW, 30 }, 18,
        Color{ 110, 115, 130, (unsigned char)(200 * ctrlFade) });

    float startFade = std::min(std::max((screenTimer - 1.6f) / 0.6f, 0.0f), 1.0f);
    float startPulse = 0.6f + 0.4f * sinf(screenTimer * 3.5f);
    DrawCenteredText("Press ENTER to start", Rectangle{ 0, 470, kScreenW, 40 }, 26,
        Color{ (unsigned char)(255 * startPulse), (unsigned char)(200 * startPulse), 50, (unsigned char)(255 * startFade) });
}

void Game::DrawTypeSelect() {
    DrawGradientBg(Color{ 10, 8, 18 }, Color{ 20, 16, 35 });

    float fadeIn = std::min(screenTimer / 0.6f, 1.0f);

    DrawCenteredText(TextFormat("SECTOR %d", currentRoom),
        Rectangle{ 0, 50, kScreenW, 40 }, 36,
        Color{ 255, 255, 255, (unsigned char)(255 * fadeIn) });

    float diffFade = std::min(std::max((screenTimer - 0.2f) / 0.5f, 0.0f), 1.0f);
    Color dc = DifficultyColor(RoomDifficulty(currentRoom));
    DrawCenteredText(TextFormat("Difficulty: %s", DifficultyName(RoomDifficulty(currentRoom))),
        Rectangle{ 0, 92, kScreenW, 24 }, 18,
        Color{ dc.r, dc.g, dc.b, (unsigned char)(255 * diffFade) });

    float promptFade = std::min(std::max((screenTimer - 0.3f) / 0.5f, 0.0f), 1.0f);
    DrawCenteredText("Choose your test type", Rectangle{ 0, 125, kScreenW, 24 }, 18,
        Color{ 160, 165, 180, (unsigned char)(200 * promptFade) });

    float cardW = 340, cardH = 340, gap = 30;
    float totalW = cardW * typeCandidates.size() + gap * (typeCandidates.size() > 0 ? typeCandidates.size() - 1 : 0);
    float startX = kScreenW / 2 - totalW / 2;
    float baseY = 180;

    for (int i = 0; i < (int)typeCandidates.size(); i++) {
        float cardFade = std::min(std::max((screenTimer - 0.2f - i * 0.15f) / 0.5f, 0.0f), 1.0f);
        float slideY = (1.0f - cardFade) * 20;
        const auto& opt = typeCandidates[i];
        Rectangle card{ startX + i * (cardW + gap), baseY + slideY, cardW, cardH };

        DrawRectangleRounded(card, 0.06f, 8, Color{ 25, 25, 38, (unsigned char)(240 * cardFade) });
        DrawRectangleRoundedLines(card, 0.06f, 8, Color{ dc.r, dc.g, dc.b, (unsigned char)(180 * cardFade) });

        Rectangle headerArea{ card.x + 12, card.y + 10, card.width - 24, 36 };
        Color headerBg{ (unsigned char)(dc.r / 4), (unsigned char)(dc.g / 4), (unsigned char)(dc.b / 4), (unsigned char)(200 * cardFade) };
        DrawRectangleRounded(headerArea, 0.3f, 4, headerBg);

        DrawCenteredText(TextFormat("[%d] %s", i + 1, opt.name.c_str()),
            Rectangle{ card.x, card.y + 10, card.width, 36 }, 22,
            Color{ dc.r, dc.g, dc.b, (unsigned char)(255 * cardFade) });

        DrawCenteredText(opt.description,
            Rectangle{ card.x + 10, card.y + 52, card.width - 20, 50 }, 14,
            Color{ 180, 185, 200, (unsigned char)(220 * cardFade) });

        Rectangle previewBg{ card.x + 14, card.y + 108, card.width - 28, 140 };
        DrawRectangleRounded(previewBg, 0.08f, 6, Color{ 15, 15, 22, (unsigned char)(220 * cardFade) });
        DrawRectangleRoundedLines(previewBg, 0.08f, 6, Color{ 70, 75, 95, (unsigned char)(120 * cardFade) });

        if (i < (int)examplePuzzles.size() && examplePuzzles[i]) {
            examplePuzzles[i]->RenderQuestion(
                Rectangle{ previewBg.x + 6, previewBg.y + 6, previewBg.width - 12, previewBg.height - 12 });
        }

        float choiceY = card.y + 260;
        if (i < (int)exampleChoices.size()) {
            const auto& ch = exampleChoices[i];
            for (int c = 0; c < 3 && c < (int)ch.size(); c++) {
                Rectangle choiceBox{ card.x + 14, choiceY + c * 24, card.width - 28, 20 };
                DrawRectangleRounded(choiceBox, 0.3f, 3, Color{ 40, 42, 58, (unsigned char)(180 * cardFade) });
                DrawCenteredText(ch[c], choiceBox, 12, Color{ 200, 205, 220, (unsigned char)(200 * cardFade) });
            }
        }
    }

    float btnFade = std::min(std::max((screenTimer - 0.6f) / 0.4f, 0.0f), 1.0f);
    float btnPulse = 0.7f + 0.3f * sinf(screenTimer * 3.0f);
    DrawCenteredText("Press 1, 2 or 3", Rectangle{ 0, baseY + cardH + 20, kScreenW, 30 }, 24,
        Color{ (unsigned char)(255 * btnPulse), (unsigned char)(200 * btnPulse), 50, (unsigned char)(255 * btnFade) });
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
    Rectangle pb = player.Bounds();
    Rectangle sabotageZone{ pb.x - 20, pb.y - 20, pb.width + 40, pb.height + 40 };
    if (sabotageAnim.texture.id != 0) {
        float btnSpriteW = (float)sabotageAnim.texture.width / sabotageAnim.frameCount;
        float btnSpriteH = (float)sabotageAnim.texture.height;
        for (auto& btn : room.sabotageButtons) {
            auto& elev = room.elevators[btn.elevatorIndex];
            bool accessible = (elev.floorLow == playerFloor) && !elev.used;
            unsigned char a = (elev.floorLow == playerFloor) ? 255 : 70;
            Color tint = elev.used ? Color{ 120, 120, 120, a } : WHITE;
            tint.a = a;
            Rectangle src = sabotageAnim.SourceRect(btn.flipped);
            DrawTexturePro(sabotageAnim.texture, src,
                { btn.rect.x, btn.rect.y, btnSpriteW, btnSpriteH },
                Vector2{0,0}, 0.0f, tint);
            bool near = CheckCollisionRecs(sabotageZone, btn.rect) && accessible;
            if (near) {
                float keyW = 18, keyH = 18;
                Rectangle keyBox{ btn.rect.x + btn.rect.width / 2 - keyW / 2, btn.rect.y - keyH - 5, keyW, keyH };
                DrawRectangleRounded(keyBox, 0.4f, 4, Color{ 100, 120, 170, 180 });
                DrawCenteredText("F", keyBox, 12, Color{ 200, 210, 235, 255 });
            }
        }
    } else {
        for (auto& btn : room.sabotageButtons) {
            auto& elev = room.elevators[btn.elevatorIndex];
            unsigned char a = (elev.floorLow == playerFloor) ? 255 : 70;
            Color c = elev.used ? Color{ 80, 80, 80, a } : Color{ 200, 60, 60, a };
            DrawRectangleRec(btn.rect, c);
            DrawRectangleLinesEx(btn.rect, 1, Color{ 20, 20, 20, a });
            bool near = CheckCollisionRecs(sabotageZone, btn.rect) && !elev.used && elev.floorLow == playerFloor;
            if (near) {
                float keyW = 18, keyH = 18;
                Rectangle keyBox{ btn.rect.x + btn.rect.width / 2 - keyW / 2, btn.rect.y - keyH - 5, keyW, keyH };
                DrawRectangleRounded(keyBox, 0.4f, 4, Color{ 100, 120, 170, 180 });
                DrawCenteredText("F", keyBox, 12, Color{ 200, 210, 235, 255 });
            }
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

        if (doorParticleActive && doorParticleAnim.texture.id != 0) {
            float pw = (float)doorParticleAnim.texture.width / doorParticleAnim.frameCount;
            float ph = (float)doorParticleAnim.texture.height;
            Rectangle psrc = doorParticleAnim.SourceRect(false);
            DrawTexturePro(doorParticleAnim.texture, psrc,
                { room.exitDoor.rect.x - pw - 2,
                  room.exitDoor.rect.y + room.exitDoor.rect.height / 2 - ph / 2, pw, ph },
                Vector2{0,0}, 0.0f, WHITE);
        }
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
        if (iconEnemy.id != 0) {
            float iw = (float)iconEnemy.width, ih = (float)iconEnemy.height;
            DrawTexture(iconEnemy,
                (int)(room.enemyDoorRect.x + room.enemyDoorRect.width / 2 - iw / 2),
                (int)(room.enemyDoorRect.y - ih - 4),
                Color{ 255, 255, 255, doorA });
        }
    }

    // Exit door icon - fades in then flashes when door opens
    if (exitDoorOpening && iconExit.id != 0) {
        float iw = (float)iconExit.width, ih = (float)iconExit.height;
        float fadeIn = std::min(exitIconTimer / 0.8f, 1.0f);
        float flash = (exitIconTimer > 0.8f) ? 0.5f + 0.5f * sinf(exitIconTimer * 6.0f) : 1.0f;
        unsigned char a = (unsigned char)(fadeIn * flash * 255);
        DrawTexture(iconExit,
            (int)(room.exitDoor.rect.x - iw * 2),
            (int)(room.exitDoor.rect.y - ih - 4),
            Color{ 255, 255, 255, a });
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
        if (s.state != SlotState::SOLVED && noiseTex[noiseFrame].id != 0) {
            Rectangle noiseSrc{ 4, 11, 126, 62 };
            Rectangle noiseDst{ dx + 4, dy + 11, 126, 62 };
            DrawTexturePro(noiseTex[noiseFrame], noiseSrc, noiseDst, {0,0}, 0.0f, Color{ 255, 255, 255, (unsigned char)(a / 8) });
        }
        float inset = 10.0f;
        float vShift = -8.0f;

        Rectangle textArea{ dx + inset, dy + inset + vShift, sw - inset * 2, sh - inset * 2 };

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
            float btnW = 140, btnH = 36, gap = 10, keyH = 18, keyW = 18;
            float totalW = btnW * 3 + gap * 2;
            float x = s.rect.x + s.rect.width / 2 - totalW / 2;
            float y = std::max(s.rect.y - 34 - btnH - keyH - 4, 54.0f);
            for (int c = 0; c < 3; c++) {
                Rectangle btn{ x + c * (btnW + gap), y, btnW, btnH };
                DrawRectangleRounded(btn, 0.3f, 6, Color{ 35, 38, 52, 240 });
                DrawRectangleRoundedLines(btn, 0.3f, 6, Color{ 100, 120, 170, 200 });
                DrawCenteredText(choices[c], btn, 14, Color{ 220, 225, 240, 255 });
                Rectangle keyBox{ btn.x + btn.width / 2 - keyW / 2, btn.y + btn.height + 5, keyW, keyH };
                DrawRectangleRounded(keyBox, 0.4f, 4, Color{ 100, 120, 170, 180 });
                DrawCenteredText(TextFormat("%d", c + 1), keyBox, 12, Color{ 200, 210, 235, 255 });
            }
        }
    }

    if (enemy.active) enemy.Render(enemyAnims, enemyFloor != playerFloor, floorAlpha(enemyFloor));
    player.Render(playerAnims);

    if (runParticleTex.id != 0) {
        float fw = (float)runParticleTex.width / 6, fh = (float)runParticleTex.height;
        for (auto& p : particles) {
            float fade = 1.0f - p.age / p.lifetime;
            unsigned char alpha = (unsigned char)(fade * 255);
            DrawTexturePro(runParticleTex, { p.frame * fw, 0, p.flipX ? -fw : fw, fh },
                { p.pos.x - fw / 2, p.pos.y - fh / 2, fw, fh },
                {0,0}, 0.0f, Color{ 255, 255, 255, alpha });
        }
    }

    DrawHUD();

    if (startCountdown > 0.0f) {
        DrawRectangle(0, 0, (int)kScreenW, (int)kScreenH, Color{ 0, 0, 0, 120 });
        int n = (int)ceilf(startCountdown);
        DrawCenteredText(n > 0 ? TextFormat("%d", n) : "GO!", Rectangle{ 0, kScreenH / 2 - 60, kScreenW, 100 }, 90, RAYWHITE);
    }

    if (messageTimer > 0 && !message.empty() && state == GameState::PLAYING) {
        DrawCenteredText(message, Rectangle{ 0, 90, kScreenW, 30 }, 22, YELLOW);
    }

    // Overlay transitions
    auto drawOverlay = [](const Texture2D& tex, float alpha) {
        if (tex.id == 0 || alpha <= 0.0f) return;
        Rectangle src{ 0, 0, (float)tex.width, (float)tex.height };
        Rectangle dst{ 0, 0, kScreenW, kScreenH };
        unsigned char a = (unsigned char)(alpha * 40);
        DrawTexturePro(tex, src, dst, {0,0}, 0.0f, Color{ 255, 255, 255, a });
    };

    if (overlayStage == 0) {
        drawOverlay(overlayTex[0], 1.0f);
    } else if (overlayStage == 1) {
        drawOverlay(overlayTex[0], 1.0f - overlayBlend);
        drawOverlay(overlayTex[1], overlayBlend);
    } else if (overlayStage == 2) {
        drawOverlay(overlayTex[1], 1.0f - overlayBlend);
        drawOverlay(overlayTex[2], overlayBlend);
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
    DrawGradientBg(Color{ 20, 6, 6 }, Color{ 40, 8, 10 });
    float fadeIn = std::min(screenTimer / 0.8f, 1.0f);
    float pulse = 0.8f + 0.2f * sinf(screenTimer * 3.0f);

    DrawCenteredText("EVALUATION FAILED", Rectangle{ 0, (int)(kScreenH / 2 - 120), kScreenW, 60 }, 52,
        Color{ (unsigned char)(255 * pulse), (unsigned char)(70 * pulse), (unsigned char)(70 * pulse), (unsigned char)(255 * fadeIn) });

    float subFade = std::min(std::max((screenTimer - 0.3f) / 0.6f, 0.0f), 1.0f);
    DrawCenteredText("You didn't prove anything.", Rectangle{ 0, (int)(kScreenH / 2 - 55), kScreenW, 30 }, 20,
        Color{ 200, 130, 130, (unsigned char)(255 * subFade) });

    float scoreFade = std::min(std::max((screenTimer - 0.6f) / 0.5f, 0.0f), 1.0f);
    DrawCenteredText(TextFormat("Final score: %d", score), Rectangle{ 0, (int)(kScreenH / 2 + 5), kScreenW, 40 }, 30,
        Color{ 255, 255, 255, (unsigned char)(255 * scoreFade) });

    float btnFade = std::min(std::max((screenTimer - 1.2f) / 0.5f, 0.0f), 1.0f);
    float btnPulse = 0.6f + 0.4f * sinf(screenTimer * 3.0f);
    DrawCenteredText("Press ENTER to return to the menu", Rectangle{ 0, (int)(kScreenH / 2 + 60), kScreenW, 40 }, 22,
        Color{ (unsigned char)(255 * btnPulse), (unsigned char)(200 * btnPulse), 50, (unsigned char)(255 * btnFade) });
}

void Game::DrawWin() {
    DrawGradientBg(Color{ 10, 14, 8 }, Color{ 16, 28, 12 });
    float fadeIn = std::min(screenTimer / 0.8f, 1.0f);
    float pulse = 0.85f + 0.15f * sinf(screenTimer * 2.0f);

    DrawCenteredText("HUMANITY: NOT OBSOLETE", Rectangle{ 0, (int)(kScreenH / 2 - 130), kScreenW, 60 }, 50,
        Color{ (unsigned char)(255 * pulse), (unsigned char)(215 * pulse), (unsigned char)(70 * pulse), (unsigned char)(255 * fadeIn) });

    float subFade = std::min(std::max((screenTimer - 0.4f) / 0.6f, 0.0f), 1.0f);
    DrawCenteredText("You proved you are better than AI.", Rectangle{ 0, (int)(kScreenH / 2 - 60), kScreenW, 30 }, 22,
        Color{ 200, 220, 200, (unsigned char)(255 * subFade) });

    float scoreFade = std::min(std::max((screenTimer - 0.8f) / 0.5f, 0.0f), 1.0f);
    DrawCenteredText(TextFormat("Final score: %d", score), Rectangle{ 0, (int)(kScreenH / 2 + 10), kScreenW, 40 }, 30,
        Color{ 255, 255, 255, (unsigned char)(255 * scoreFade) });

    float btnFade = std::min(std::max((screenTimer - 1.4f) / 0.5f, 0.0f), 1.0f);
    float btnPulse = 0.6f + 0.4f * sinf(screenTimer * 3.0f);
    DrawCenteredText("Press ENTER to return to the menu", Rectangle{ 0, (int)(kScreenH / 2 + 70), kScreenW, 40 }, 22,
        Color{ (unsigned char)(255 * btnPulse), (unsigned char)(200 * btnPulse), 50, (unsigned char)(255 * btnFade) });
}