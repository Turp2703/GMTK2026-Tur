#pragma once
#include "puzzle.h"

// =============================================================================
// Small helpers shared across puzzle types
// =============================================================================
inline int Gcd(int a, int b) { return b == 0 ? std::abs(a) : Gcd(b, a % b); }

inline std::string ToRoman(int num) {
    static const std::vector<std::pair<int, std::string>> table = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"},
        {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"}
    };
    if (num <= 0) return "N/A";
    std::string result;
    for (auto& [value, symbol] : table) {
        while (num >= value) { result += symbol; num -= value; }
    }
    return result;
}

inline std::string ToHex(int value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%X", value);
    return std::string(buf);
}

inline std::string ToBinary(int value) {
    if (value == 0) return "0";
    std::string s;
    int v = value;
    while (v > 0) { s = char('0' + (v & 1)) + s; v >>= 1; }
    return s;
}

// Generates a set of `count` distinct integers near `correct`, excluding
// `correct` itself and staying >= floor. Used to build plausible decoys.
inline std::vector<int> NearbyDistinctInts(int correct, int count, int minSpread, int maxSpread, int floorValue = 0) {
    std::vector<int> out;
    int guard = 0;
    while ((int)out.size() < count && guard < 200) {
        guard++;
        int sign = RandInt(0, 1) ? 1 : -1;
        int offset = sign * RandInt(minSpread, maxSpread);
        int candidate = correct + offset;
        if (candidate < floorValue) candidate = correct + std::abs(offset);
        if (candidate == correct) continue;
        if (std::find(out.begin(), out.end(), candidate) != out.end()) continue;
        out.push_back(candidate);
    }
    // Fallback in the unlikely case we couldn't find enough distinct values.
    int fallback = correct + maxSpread + 1;
    while ((int)out.size() < count) { if (fallback != correct) out.push_back(fallback); fallback++; }
    return out;
}

// =============================================================================
// EASY: Math expression (add, sub, mult) -> Result
// =============================================================================
class MathEasyPuzzle : public Puzzle {
public:
    MathEasyPuzzle() { difficulty = Difficulty::EASY; }

    void Generate() override {
        a = RandInt(1, 20);
        b = RandInt(1, 20);
        int op = RandInt(0, 2);
        if (op == 0) { symbol = "+"; result = a + b; }
        else if (op == 1) { symbol = "-"; if (b > a) std::swap(a, b); result = a - b; }
        else { a = RandInt(1, 12); b = RandInt(1, 12); symbol = "x"; result = a * b; }

        auto decoys = NearbyDistinctInts(result, 2, 1, 6, 0);
        std::array<std::string, 3> choices = { TextFormat("%d", result), TextFormat("%d", decoys[0]), TextFormat("%d", decoys[1]) };
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        std::string expr = TextFormat("%d %s %d", a, symbol.c_str(), b);
        DrawCenteredText(expr, bounds, 40, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 10; }
    std::string TypeName() const override { return "Math"; }

private:
    int a = 0, b = 0, result = 0;
    std::string symbol = "+";
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// EASY: Time - Clock hands -> digital time
// =============================================================================
class TimeClockPuzzle : public Puzzle {
public:
    TimeClockPuzzle() { difficulty = Difficulty::EASY; }

    void Generate() override {
        hour = RandInt(1, 12);
        minute = RandInt(0, 11) * 5;

        std::array<std::string, 3> choices;
        choices[0] = TextFormat("%02d:%02d", hour, minute);

        // Decoys: shift the minute hand by some 5-minute steps, or the hour.
        int mIdx = minute / 5;
        int d1 = (mIdx + RandInt(1, 5)) % 12;
        int d2 = (mIdx + RandInt(6, 10)) % 12;
        int h1 = hour, h2 = ((hour) % 12) + 1;
        choices[1] = TextFormat("%02d:%02d", h1, d1 * 5);
        choices[2] = TextFormat("%02d:%02d", h2, d2 * 5);
        if (choices[1] == choices[0]) choices[1] = TextFormat("%02d:%02d", h1, ((d1 + 1) % 12) * 5);
        if (choices[2] == choices[0] || choices[2] == choices[1]) choices[2] = TextFormat("%02d:%02d", h2, ((d2 + 2) % 12) * 5);

        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        Vector2 center{ bounds.x + bounds.width / 2, bounds.y + bounds.height / 2 };
        float radius = std::min(bounds.width, bounds.height) / 2.0f - 12;
        DrawCircleLines((int)center.x, (int)center.y, radius, RAYWHITE);
        for (int i = 0; i < 12; i++) {
            float ang = i * 30.0f * DEG2RAD;
            Vector2 p{ center.x + sinf(ang) * (radius - 6), center.y - cosf(ang) * (radius - 6) };
            DrawCircleV(p, 2, GRAY);
        }
        float hourAngle = ((hour % 12) + minute / 60.0f) * 30.0f * DEG2RAD;
        float minAngle = minute * 6.0f * DEG2RAD;
        Vector2 hourEnd{ center.x + sinf(hourAngle) * radius * 0.5f, center.y - cosf(hourAngle) * radius * 0.5f };
        Vector2 minEnd{ center.x + sinf(minAngle) * radius * 0.8f, center.y - cosf(minAngle) * radius * 0.8f };
        DrawLineEx(center, hourEnd, 4, RAYWHITE);
        DrawLineEx(center, minEnd, 2, SKYBLUE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 10; }
    std::string TypeName() const override { return "Time"; }

private:
    int hour = 12, minute = 0;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// EASY: Unit conversion (meter -> other length unit)
// =============================================================================
class UnitConversionPuzzle : public Puzzle {
public:
    UnitConversionPuzzle() { difficulty = Difficulty::EASY; }

    void Generate() override {
        meters = RandInt(1, 20);
        static const std::vector<std::pair<std::string, double>> units = {
            {"km", 0.001}, {"cm", 100.0}, {"mm", 1000.0}
        };
        auto pick = RandChoice(units);
        unitName = pick.first;
        double correct = meters * pick.second;

        auto fmt = [](double v, const char* unit) {
            int decimals = 0;
            double absVal = fabs(v);
            if (absVal > 0.0 && absVal < 1.0) {
                decimals = (int)ceil(-log10(absVal)) + 1;
                decimals = std::clamp(decimals, 1, 6);
            }
            char buf[64];
            TextCopy(buf, TextFormat("%.*f %s", decimals, v, unit));
            return std::string(buf);
        };

        std::array<double, 3> wrongMult = { 10.0, 0.1, 2.0 };
        std::shuffle(wrongMult.begin(), wrongMult.end(), RNG());
        std::array<std::string, 3> choices;
        choices[0] = fmt(correct, unitName.c_str());
        choices[1] = fmt(correct * wrongMult[0], unitName.c_str());
        choices[2] = fmt(correct * wrongMult[1], unitName.c_str());
        if (choices[1] == choices[0] || choices[2] == choices[0]) {
            choices[2] = fmt(correct * wrongMult[2], unitName.c_str());
        }
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        DrawCenteredText(TextFormat("%d m", meters), bounds, 40, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 10; }
    std::string TypeName() const override { return "Units"; }

private:
    int meters = 1;
    std::string unitName = "km";
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// MEDIUM: Math expression with fractions -> Result (as reduced fraction)
// =============================================================================
class MathFractionPuzzle : public Puzzle {
public:
    MathFractionPuzzle() { difficulty = Difficulty::MEDIUM; }

    void Generate() override {
        n1 = RandInt(1, 5); d1 = RandInt(2, 6);
        n2 = RandInt(1, 5); d2 = RandInt(2, 6);
        int op = RandInt(0, 1);
        int rn, rd;
        if (op == 0) { symbol = "+"; rn = n1 * d2 + n2 * d1; rd = d1 * d2; }
        else { symbol = "-"; if ((double)n1 / d1 < (double)n2 / d2) { std::swap(n1, n2); std::swap(d1, d2); }
               rn = n1 * d2 - n2 * d1; rd = d1 * d2; }
        int g = Gcd(rn, rd); if (g == 0) g = 1;
        rn /= g; rd /= g;

        auto fmt = [](int n, int d) { return (d == 1) ? std::string(TextFormat("%d", n)) : std::string(TextFormat("%d/%d", n, d)); };
        std::array<std::string, 3> choices;
        choices[0] = fmt(rn, rd);
        // Decoy: common student mistake of not reducing + adding denominators directly.
        int wrongRn1 = (op == 0) ? (n1 + n2) : std::abs(n1 - n2);
        int wrongRd1 = d1 + d2;
        choices[1] = fmt(wrongRn1, wrongRd1);
        choices[2] = fmt(rn + (RandInt(0, 1) ? 1 : -1), rd);
        if (choices[1] == choices[0]) choices[1] = fmt(wrongRn1 + 1, wrongRd1);
        if (choices[2] == choices[0] || choices[2] == choices[1]) choices[2] = fmt(rn, rd + 1);
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        std::string expr = TextFormat("%d/%d %s %d/%d", n1, d1, symbol.c_str(), n2, d2);
        DrawCenteredText(expr, bounds, 30, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 20; }
    std::string TypeName() const override { return "Fractions"; }

private:
    int n1 = 1, d1 = 2, n2 = 1, d2 = 2;
    std::string symbol = "+";
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// MEDIUM: Binary -> Hex
// =============================================================================
class BinaryHexPuzzle : public Puzzle {
public:
    BinaryHexPuzzle() { difficulty = Difficulty::MEDIUM; }
    void Generate() override {
        value = RandInt(1, 255);
        auto decoys = NearbyDistinctInts(value, 2, 1, 20, 0);
        std::array<std::string, 3> choices = { ToHex(value), ToHex(decoys[0]), ToHex(decoys[1]) };
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        DrawCenteredText(ToBinary(value), bounds, 30, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 20; }
    std::string TypeName() const override { return "Binary-Hex"; }

private:
    int value = 0;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// MEDIUM: Decimal -> Roman
// =============================================================================
class DecimalRomanPuzzle : public Puzzle {
public:
    DecimalRomanPuzzle() { difficulty = Difficulty::MEDIUM; }
    void Generate() override {
        value = RandInt(1, 399);
        auto decoys = NearbyDistinctInts(value, 2, 1, 15, 1);
        std::array<std::string, 3> choices = { ToRoman(value), ToRoman(decoys[0]), ToRoman(decoys[1]) };
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        DrawCenteredText(TextFormat("%d", value), bounds, 40, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 20; }
    std::string TypeName() const override { return "Roman"; }

private:
    int value = 1;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// MEDIUM: Boolean expression -> Result
// =============================================================================
class BooleanExprPuzzle : public Puzzle {
public:
    BooleanExprPuzzle() { difficulty = Difficulty::MEDIUM; }

    void Generate() override {
        bool a = RandInt(0, 1);
        bool b = RandInt(0, 1);
        int op = RandInt(0, 2);
        if (op == 0) { symbol = "AND"; result = a && b; }
        else if (op == 1) { symbol = "OR"; result = a || b; }
        else { symbol = "XOR"; result = a != b; }
        av = a; bv = b;

        // Only two real outcomes exist (TRUE/FALSE), so the third choice is
        // always a fixed decoy that can never be correct.
        std::array<std::string, 3> choices = { result ? "TRUE" : "FALSE", result ? "FALSE" : "TRUE", "UNKNOWN" };
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        std::string expr = TextFormat("%s %s %s", av ? "TRUE" : "FALSE", symbol.c_str(), bv ? "TRUE" : "FALSE");
        DrawCenteredText(expr, bounds, 24, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 20; }
    std::string TypeName() const override { return "Boolean"; }

private:
    bool av = false, bv = false, result = false;
    std::string symbol = "AND";
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// MEDIUM: Chemical symbol -> Element name
// =============================================================================
class ChemSymbolPuzzle : public Puzzle {
public:
    ChemSymbolPuzzle() { difficulty = Difficulty::MEDIUM; }

    void Generate() override {
        static const std::vector<std::pair<std::string, std::string>> elements = {
            {"H", "Hydrogen"}, {"He", "Helium"}, {"Li", "Lithium"}, {"C", "Carbon"},
            {"N", "Nitrogen"}, {"O", "Oxygen"}, {"Na", "Sodium"}, {"Mg", "Magnesium"},
            {"Al", "Aluminum"}, {"Si", "Silicon"}, {"K", "Potassium"}, {"Ca", "Calcium"},
            {"Fe", "Iron"}, {"Cu", "Copper"}, {"Zn", "Zinc"}, {"Ag", "Silver"},
            {"Au", "Gold"}, {"Pb", "Lead"}, {"Cl", "Chlorine"}, {"S", "Sulfur"}
        };
        int idx = RandInt(0, (int)elements.size() - 1);
        symbol = elements[idx].first;
        std::string correctName = elements[idx].second;

        std::array<std::string, 3> choices;
        choices[0] = correctName;
        int d1, d2;
        do { d1 = RandInt(0, (int)elements.size() - 1); } while (d1 == idx);
        do { d2 = RandInt(0, (int)elements.size() - 1); } while (d2 == idx || d2 == d1);
        choices[1] = elements[d1].second;
        choices[2] = elements[d2].second;
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        DrawCenteredText(symbol, bounds, 44, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 20; }
    std::string TypeName() const override { return "Chemistry"; }

private:
    std::string symbol;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// HARD: Decimal -> Hex
// =============================================================================
class DecimalHexPuzzle : public Puzzle {
public:
    DecimalHexPuzzle() { difficulty = Difficulty::HARD; }
    void Generate() override {
        value = RandInt(16, 4095);
        auto decoys = NearbyDistinctInts(value, 2, 1, 40, 0);
        std::array<std::string, 3> choices = { ToHex(value), ToHex(decoys[0]), ToHex(decoys[1]) };
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        DrawCenteredText(TextFormat("%d", value), bounds, 38, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 35; }
    std::string TypeName() const override { return "Decimal-Hex"; }

private:
    int value = 16;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// HARD: RGB -> Color swatch (question shows the swatch, choices are RGB text)
// =============================================================================
class RGBColorPuzzle : public Puzzle {
public:
    RGBColorPuzzle() { difficulty = Difficulty::HARD; }
    void Generate() override {
        r = RandInt(0, 255); g = RandInt(0, 255); b = RandInt(0, 255);
        auto fmt = [](int r, int g, int b) { return std::string(TextFormat("RGB(%d,%d,%d)", r, g, b)); };
        std::array<std::string, 3> choices;
        choices[0] = fmt(r, g, b);
        auto rd = NearbyDistinctInts(r, 1, 20, 60, 0)[0];
        auto gd = NearbyDistinctInts(g, 1, 20, 60, 0)[0];
        auto bd = NearbyDistinctInts(b, 1, 20, 60, 0)[0];
        choices[1] = fmt(Clamp((float)rd, 0, 255), g, b);
        choices[2] = fmt(r, Clamp((float)gd, 0, 255), Clamp((float)bd, 0, 255));
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        Rectangle swatch{ bounds.x + bounds.width / 2 - 40, bounds.y + bounds.height / 2 - 30, 80, 60 };
        DrawRectangleRec(swatch, Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 });
        DrawRectangleLinesEx(swatch, 2, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 35; }
    std::string TypeName() const override { return "RGB Color"; }

private:
    int r = 0, g = 0, b = 0;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// HARD: Linear equation -> X value
// =============================================================================
class EquationPuzzle : public Puzzle {
public:
    EquationPuzzle() { difficulty = Difficulty::HARD; }

    void Generate() override {
        x = RandInt(-10, 10);
        a = RandInt(2, 9);
        b = RandInt(-15, 15);
        c = a * x + b;

        auto decoys = NearbyDistinctInts(x, 2, 1, 6, -999);
        std::array<std::string, 3> choices = {
            TextFormat("x = %d", x), TextFormat("x = %d", decoys[0]), TextFormat("x = %d", decoys[1])
        };
        correctIndex = ShuffleChoices(choices);
        finalChoices = choices;
    }

    void RenderQuestion(Rectangle bounds) const override {
        std::string expr = TextFormat("%dx %s %d = %d", a, b >= 0 ? "+" : "-", std::abs(b), c);
        DrawCenteredText(expr, bounds, 26, RAYWHITE);
    }
    std::array<std::string, 3> GetChoices() const override { return finalChoices; }
    int GetCorrectIndex() const override { return correctIndex; }
    int GetPoints() const override { return 35; }
    std::string TypeName() const override { return "Equation"; }

private:
    int x = 0, a = 1, b = 0, c = 0;
    int correctIndex = 0;
    std::array<std::string, 3> finalChoices;
};

// =============================================================================
// Puzzle type registry - one room uses exactly one type.
// =============================================================================
// PuzzleFactory is defined in Puzzle.h

struct PuzzleTypeInfo {
    std::string name;
    std::string description;
    Difficulty difficulty;
    PuzzleFactory factory;
};

template <typename T>
inline PuzzleFactory MakeFactory() {
    return []() -> PuzzlePtr {
        auto p = std::make_shared<T>();
        p->Generate();
        return p;
    };
}

inline const std::vector<PuzzleTypeInfo>& AllPuzzleTypes() {
    static const std::vector<PuzzleTypeInfo> types = {
        { "Math",         "Addition, subtraction, multiplication -> Result", Difficulty::EASY,   MakeFactory<MathEasyPuzzle>() },
        { "Time",         "Clock hands -> Digital time",                     Difficulty::EASY,   MakeFactory<TimeClockPuzzle>() },
        { "Units",        "Meters -> another length unit",                   Difficulty::EASY,   MakeFactory<UnitConversionPuzzle>() },
        { "Fractions",    "Fraction addition/subtraction -> Result",         Difficulty::MEDIUM, MakeFactory<MathFractionPuzzle>() },
        { "Binary-Hex",   "Binary number -> Hexadecimal",                    Difficulty::MEDIUM, MakeFactory<BinaryHexPuzzle>() },
        { "Roman",        "Decimal number -> Roman numerals",                Difficulty::MEDIUM, MakeFactory<DecimalRomanPuzzle>() },
        { "Boolean",      "Boolean expression -> Result",                    Difficulty::MEDIUM, MakeFactory<BooleanExprPuzzle>() },
        { "Chemistry",    "Chemical symbol -> Element name",                 Difficulty::MEDIUM, MakeFactory<ChemSymbolPuzzle>() },
        { "Decimal-Hex",  "Decimal number -> Hexadecimal",                   Difficulty::HARD,   MakeFactory<DecimalHexPuzzle>() },
        { "RGB Color",    "RGB values -> Color swatch",                      Difficulty::HARD,   MakeFactory<RGBColorPuzzle>() },
        { "Equation",     "Linear equation -> Value of X",                   Difficulty::HARD,   MakeFactory<EquationPuzzle>() },
    };
    return types;
}

inline std::vector<const PuzzleTypeInfo*> PuzzleTypesForDifficulty(Difficulty diff) {
    std::vector<const PuzzleTypeInfo*> v;
    for (auto& t : AllPuzzleTypes()) if (t.difficulty == diff) v.push_back(&t);
    return v;
}

inline const PuzzleTypeInfo& PickRandomTypeInfo(Difficulty diff) {
    auto v = PuzzleTypesForDifficulty(diff);
    return *v[RandInt(0, (int)v.size() - 1)];
}
