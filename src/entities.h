#pragma once
#include "Common.h"
#include "Puzzle.h"

struct Platform {
    Rectangle rect;
    int floor = 0;
};

struct Elevator {
    Rectangle rect;
    int floorLow = 0;
    int floorHigh = 1;
    bool active = true;
    bool used = false;        // sabotage is a one-time action per elevator
    float reactivateTimer = 0.0f;

    void Update(float dt) {
        if (!active) {
            reactivateTimer -= dt;
            if (reactivateTimer <= 0.0f) active = true;
        }
    }
};

struct SabotageButton {
    Rectangle rect;
    int elevatorIndex = -1;
    float holdTime = 0.0f; // accumulates while F is held nearby
    static constexpr float kRequiredHold = 3.0f;
};

struct Door {
    Rectangle rect;
    int floor = 0;
    bool isExit = false; // false = entrance (A), true = exit (B)
};

enum class SlotState { UNSOLVED, SOLVED, WRONG_COOLDOWN, BROKEN };

struct PuzzleSlot {
    Rectangle rect;
    int floor = 0;
    bool isBonus = false;     // the "N" next-difficulty puzzle; not required for the door
    SlotState state = SlotState::UNSOLVED;
    PuzzlePtr puzzle;
    PuzzleFactory regenerate; // used to create a fresh instance after a wrong answer
    float cooldownTimer = 0.0f;
    static constexpr float kCooldown = 5.0f;
};

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------
class Player {
public:
    enum Anim { ANIM_SPAWN = 0, ANIM_IDLE, ANIM_RUN, ANIM_LADDER, ANIM_FALL, ANIM_COUNT };

    Vector2 pos{ 0, 0 };
    Vector2 size{ 32, 36 };
    float speed = 220.0f;
    float climbSpeed = 160.0f;
    bool onElevator = false;
    bool facingRight = true;
    float gravity = 900.0f;
    Vector2 velocity{ 0, 0 };
    bool controlsEnabled = true;

    bool grounded = false;
    bool climbing = false;
    bool wasMoving = false;

    Rectangle Bounds() const { return { pos.x, pos.y, size.x, size.y }; }

    void Update(float dt, const std::vector<Platform>& platforms, const std::vector<Elevator>& elevators, float screenW, float screenH) {
        Rectangle elevatorCheck{ pos.x, pos.y, size.x, size.y + 6 };
        onElevator = false;
        const Elevator* currentElevator = nullptr;
        for (auto& e : elevators) {
            if (CheckCollisionRecs(elevatorCheck, e.rect)) { onElevator = e.active; currentElevator = &e; break; }
        }

        float moveX = 0;
        if (controlsEnabled) {
            if (IsKeyDown(KEY_A)) { moveX -= 1; facingRight = false; }
            if (IsKeyDown(KEY_D)) { moveX += 1; facingRight = true; }
        }

        pos.x += moveX * speed * dt;
        pos.x = Clamp(pos.x, 0, screenW - size.x);

        climbing = controlsEnabled && onElevator && (IsKeyDown(KEY_W) || IsKeyDown(KEY_S));

        grounded = false;

        if (climbing) {
            float moveY = 0;
            if (IsKeyDown(KEY_W)) moveY -= 1;
            if (IsKeyDown(KEY_S)) moveY += 1;
            pos.y += moveY * climbSpeed * dt;
            velocity.y = 0;
        } else {
            velocity.y += gravity * dt;
            pos.y += velocity.y * dt;

            Rectangle after = Bounds();
            for (auto& p : platforms) {
                if (CheckCollisionRecs(after, p.rect)) {
                    float feet = after.y + after.height;
                    float prevFeet = feet - velocity.y * dt;
                    if (velocity.y >= 0 && prevFeet <= p.rect.y + 6) {
                        pos.y = p.rect.y - size.y;
                        velocity.y = 0;
                        grounded = true;
                    }
                    after = Bounds();
                }
            }
        }

        if (pos.y + size.y > screenH) { pos.y = screenH - size.y; velocity.y = 0; grounded = true; }
        if (pos.y < 0) pos.y = 0;

        wasMoving = (moveX != 0 || (climbing && (IsKeyDown(KEY_W) || IsKeyDown(KEY_S))));
        (void)currentElevator;
    }

    void Render(const std::array<SpriteAnimation, ANIM_COUNT>& textures) const {
        int idx = ANIM_IDLE;
        if (!controlsEnabled) {
            idx = ANIM_SPAWN;
        } else if (climbing) {
            idx = ANIM_LADDER;
        } else if (!grounded) {
            idx = ANIM_FALL;
        } else if (wasMoving) {
            idx = ANIM_RUN;
        }

        const auto& anim = textures[idx];
        if (anim.texture.id == 0) {
            Rectangle b = Bounds();
            DrawRectangleRec(b, Color{ 90, 170, 250, 255 });
            DrawRectangleLinesEx(b, 2, Color{ 20, 60, 120, 255 });
            return;
        }

        Rectangle src = anim.SourceRect(!facingRight);
        float spriteW = (float)anim.texture.width / anim.frameCount;
        float spriteH = (float)anim.texture.height;
        float drawX = pos.x + size.x / 2.0f - spriteW / 2.0f;
        float drawY = pos.y + size.y - spriteH;
        Rectangle dst = { drawX, drawY, spriteW, spriteH };
        DrawTexturePro(anim.texture, src, dst, Vector2{0, 0}, 0.0f, WHITE);
    }
};

// ---------------------------------------------------------------------------
// Enemy - a containment-unit robot that spawns after the timer runs out.
// It always knows the player's true position (the player just can't see it
// while it's on another floor), and can ride elevators the same way the
// player does.
// ---------------------------------------------------------------------------
class Enemy {
public:
    enum Anim { ANIM_SPAWN = 0, ANIM_RUN, ANIM_LADDER, ANIM_COUNT };

    Vector2 pos{ 0, 0 };
    Vector2 size{ 32, 80 };
    float speed = 150.0f;
    float climbSpeed = 130.0f;
    bool active = false;
    bool facingRight = true;
    bool grounded = false;
    bool climbing = false;
    bool spawning = false;

    Rectangle Bounds() const { return { pos.x, pos.y, size.x, size.y }; }

    void Spawn(Vector2 at) { pos = at; active = true; spawning = true; }

    void Update(float dt, const Player& player, const std::vector<Platform>& platforms, const std::vector<Elevator>& elevators, float screenH) {
        if (!active) return;

        if (spawning) return;

        float dx = (player.pos.x + player.size.x / 2) - (pos.x + size.x / 2);
        float dy = (player.pos.y + player.size.y / 2) - (pos.y + size.y / 2);

        if (fabs(dx) > 4) { pos.x += (dx > 0 ? 1 : -1) * speed * dt; facingRight = dx > 0; }

        Rectangle elevatorCheck{ pos.x, pos.y, size.x, size.y + 6 };
        climbing = false;
        for (auto& e : elevators) {
            if (e.active && CheckCollisionRecs(elevatorCheck, e.rect)) { climbing = true; break; }
        }

        grounded = false;

        if (climbing && fabs(dy) > 6) {
            pos.y += (dy > 0 ? 1 : -1) * climbSpeed * dt;
        } else if (!climbing) {
            static const float gravity = 900.0f;
            vy += gravity * dt;
            pos.y += vy * dt;
            for (auto& p : platforms) {
                if (CheckCollisionRecs(Bounds(), p.rect)) {
                    float feet = pos.y + size.y;
                    if (vy >= 0 && feet - vy * dt <= p.rect.y + 6) {
                        pos.y = p.rect.y - size.y;
                        vy = 0;
                        grounded = true;
                    }
                }
            }
            if (pos.y + size.y > screenH) { pos.y = screenH - size.y; vy = 0; grounded = true; }
        }
    }

    bool CatchesPlayer(const Player& player) const {
        return active && CheckCollisionRecs(Bounds(), player.Bounds());
    }

    void Render(const std::array<SpriteAnimation, ANIM_COUNT>& textures) const {
        if (!active) return;

        int idx = ANIM_RUN;
        if (spawning) {
            idx = ANIM_SPAWN;
        } else if (climbing) {
            idx = ANIM_LADDER;
        }

        const auto& anim = textures[idx];
        if (anim.texture.id == 0) {
            Rectangle b = Bounds();
            DrawRectangleRec(b, Color{ 200, 50, 50, 255 });
            DrawRectangleLinesEx(b, 2, Color{ 110, 10, 10, 255 });
            DrawCircle((int)(b.x + b.width / 2), (int)(b.y + 10), 5, Color{ 255, 60, 60, 255 });
            DrawCircle((int)(b.x + b.width / 2), (int)(b.y + 10), 2, WHITE);
            return;
        }

        Rectangle src = anim.SourceRect(!facingRight);
        float spriteW = (float)anim.texture.width / anim.frameCount;
        float spriteH = (float)anim.texture.height;
        float drawX = pos.x + size.x / 2.0f - spriteW / 2.0f;
        float drawY = pos.y + size.y - spriteH;
        Rectangle dst = { drawX, drawY, spriteW, spriteH };
        DrawTexturePro(anim.texture, src, dst, Vector2{0, 0}, 0.0f, WHITE);
    }

private:
    float vy = 0;
};
