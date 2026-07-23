#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <sstream>
#include <cmath>
#include <array>
#include <functional>
#include <algorithm>

// ---------------------------------------------------------------------------
// Global RNG helpers
// ---------------------------------------------------------------------------
inline std::mt19937& RNG() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

inline int RandInt(int lo, int hi) { // inclusive
    std::uniform_int_distribution<int> d(lo, hi);
    return d(RNG());
}

inline float RandFloat(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(RNG());
}

inline bool RollChance(float probability0to1) {
    return RandFloat(0.0f, 1.0f) < probability0to1;
}

template <typename T>
inline const T& RandChoice(const std::vector<T>& v) {
    return v[RandInt(0, (int)v.size() - 1)];
}

inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

enum class Difficulty { EASY = 0, MEDIUM = 1, HARD = 2 };

inline const char* DifficultyName(Difficulty d) {
    switch (d) {
        case Difficulty::EASY: return "EASY";
        case Difficulty::MEDIUM: return "MEDIUM";
        case Difficulty::HARD: return "HARD";
    }
    return "?";
}

inline Color DifficultyColor(Difficulty d) {
    switch (d) {
        case Difficulty::EASY: return Color{ 88, 189, 106, 255 };
        case Difficulty::MEDIUM: return Color{ 235, 179, 41, 255 };
        case Difficulty::HARD: return Color{ 224, 78, 78, 255 };
    }
    return WHITE;
}

inline Difficulty NextDifficulty(Difficulty d) {
    if (d == Difficulty::EASY) return Difficulty::MEDIUM;
    if (d == Difficulty::MEDIUM) return Difficulty::HARD;
    return Difficulty::HARD;
}
