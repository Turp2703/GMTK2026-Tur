#pragma once
#include "Common.h"
#include "Entities.h"
#include "Puzzles.h"

constexpr float kScreenW = 1280.0f;
constexpr float kScreenH = 720.0f;
constexpr int kFloorCount = 4;

struct RoomLayout {
    std::vector<Platform> platforms;
    std::vector<Elevator> elevators;
    std::vector<SabotageButton> sabotageButtons;
    std::vector<PuzzleSlot> puzzleSlots; // 8 required "P" slots + 1 bonus "N" slot
    Door entranceDoor;
    Door exitDoor;
    Vector2 playerSpawn{ 0, 0 };
    Vector2 enemySpawn{ 0, 0 };
    int requiredSolves = 0; // number of non-broken required puzzle slots

    float floorY[kFloorCount]; // top-of-platform Y per floor, 0 = ground

    int FloorForY(float feetY) const {
        int best = 0;
        float bestDist = 1e9f;
        for (int f = 0; f < kFloorCount; f++) {
            float d = fabsf(feetY - floorY[f]);
            if (d < bestDist) { bestDist = d; best = f; }
        }
        return best;
    }
};

// Column layout (7 slots, matching the ASCII room diagram):
//   0=A door, 1=P, 2=Elevator, 3=P/N/S, 4=Elevator, 5=P, 6=B door
inline float ColumnX(int col) {
    const float margin = 60.0f;
    const float usable = kScreenW - margin * 2;
    const float slotW = usable / 7.0f;
    return margin + slotW * (col + 0.5f);
}

inline RoomLayout GenerateRoom(int roomNumber, const PuzzleTypeInfo& chosenType) {
    RoomLayout room;
    Difficulty diff = chosenType.difficulty;
    Difficulty nextDiff = NextDifficulty(diff);
    bool isLastRoom = (diff == Difficulty::HARD); // no "next difficulty" beyond hard

    const float platformThickness = 20.0f;
    const float floorGap = 160.0f;
    for (int f = 0; f < kFloorCount; f++) {
        room.floorY[f] = kScreenH - 40.0f - f * floorGap;
        room.platforms.push_back({ Rectangle{ 0, room.floorY[f], kScreenW, platformThickness }, f });
    }

    // Elevators: one shaft at column 2 and one at column 4, for every gap
    // between adjacent floors. (All present for now - the "50% chance for
    // the second one" rule is left for later.)
    const float elevatorWidth = 34.0f;
    for (int f = 0; f < kFloorCount - 1; f++) {
        for (int col : { 2, 4 }) {
            float cx = ColumnX(col);
            Elevator e;
            e.floorLow = f;
            e.floorHigh = f + 1;
            e.rect = Rectangle{ cx - elevatorWidth / 2, room.floorY[f + 1], elevatorWidth, (room.floorY[f] + platformThickness) - room.floorY[f + 1] };
            room.elevators.push_back(e);
        }
    }

    // Sabotage buttons, one per elevator, placed beside the shaft at its
    // lower floor.
    for (int i = 0; i < (int)room.elevators.size(); i++) {
        auto& e = room.elevators[i];
        SabotageButton btn;
        btn.elevatorIndex = i;
        btn.rect = Rectangle{ e.rect.x - 40, room.floorY[e.floorLow] - 26, 30, 26 };
        room.sabotageButtons.push_back(btn);
    }

    // Doors on the ground floor.
    room.entranceDoor = { Rectangle{ ColumnX(0) - 24, room.floorY[0] - 60, 48, 60 }, 0, false };
    room.exitDoor = { Rectangle{ ColumnX(6) - 24, room.floorY[0] - 60, 48, 60 }, 0, true };
    room.playerSpawn = { room.entranceDoor.rect.x + 10, room.floorY[0] - 36 };

    // Puzzle slot helper.
    const float slotW = 90, slotH = 70;
    auto makeSlot = [&](int col, int floor, bool bonus) {
        PuzzleSlot slot;
        slot.floor = floor;
        slot.isBonus = bonus;
        slot.rect = Rectangle{ ColumnX(col) - slotW / 2, room.floorY[floor] - slotH, slotW, slotH };
        slot.regenerate = bonus ? PuzzleFactory([nextDiff]() { return PickRandomTypeInfo(nextDiff).factory(); })
                                 : chosenType.factory;
        slot.puzzle = slot.regenerate();
        slot.state = SlotState::UNSOLVED;
        return slot;
    };

    // Floor 0 (ground): P at column 3.
    room.puzzleSlots.push_back(makeSlot(3, 0, false));
    // Floor 1: P at columns 1, 3, 5.
    room.puzzleSlots.push_back(makeSlot(1, 1, false));
    room.puzzleSlots.push_back(makeSlot(3, 1, false));
    room.puzzleSlots.push_back(makeSlot(5, 1, false));
    // Floor 2: P at columns 1, 5; N (bonus, next difficulty) at column 3.
    room.puzzleSlots.push_back(makeSlot(1, 2, false));
    room.puzzleSlots.push_back(makeSlot(5, 2, false));
    room.puzzleSlots.push_back(makeSlot(3, 2, true));
    // Floor 3: P at columns 1, 5. Column 3 is the enemy spawn door (S), not a puzzle.
    room.puzzleSlots.push_back(makeSlot(1, 3, false));
    room.puzzleSlots.push_back(makeSlot(5, 3, false));

    room.enemySpawn = { ColumnX(3) - 16, room.floorY[3] - 80 };

    // Break ~5% of the 8 required slots, guaranteeing at least one stays
    // solvable. The bonus "N" slot is never randomly broken - but on the
    // last room (hard difficulty) there is no difficulty above hard, so it
    // is simply shown broken instead of being a special case.
    std::vector<int> requiredIndices;
    for (int i = 0; i < (int)room.puzzleSlots.size(); i++) if (!room.puzzleSlots[i].isBonus) requiredIndices.push_back(i);

    int guaranteedSafe = requiredIndices[RandInt(0, (int)requiredIndices.size() - 1)];
    for (int idx : requiredIndices) {
        if (idx == guaranteedSafe) continue;
        if (RollChance(0.05f)) room.puzzleSlots[idx].state = SlotState::BROKEN;
    }

    for (auto& slot : room.puzzleSlots) {
        if (slot.isBonus && isLastRoom) slot.state = SlotState::BROKEN;
    }

    room.requiredSolves = 0;
    for (auto& slot : room.puzzleSlots) {
        if (!slot.isBonus && slot.state != SlotState::BROKEN) room.requiredSolves++;
    }

    return room;
}
