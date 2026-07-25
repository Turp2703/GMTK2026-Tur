#pragma once
#include "Common.h"

// A puzzle is now single-screen: it shows one question and offers three
// possible text answers (one correct, two decoys). The player presses
// 1/2/3 to pick.
class Puzzle {
public:
    virtual ~Puzzle() = default;

    // Fill internal state with a new random question + 3 shuffled choices.
    virtual void Generate() = 0;

    // Draw the question/prompt inside bounds (this is the part that should
    // look good - analog clock hands, an RGB swatch, etc).
    virtual void RenderQuestion(Rectangle bounds) const = 0;

    // Three short text labels shown as buttons above the screen.
    virtual std::array<std::string, 3> GetChoices() const = 0;

    // Index (0-2) into GetChoices() that is correct.
    virtual int GetCorrectIndex() const = 0;

    // Points earned for answering correctly, scaled with difficulty.
    virtual int GetPoints() const = 0;

    virtual std::string TypeName() const = 0;

    Difficulty GetDifficulty() const { return difficulty; }

protected:
    Difficulty difficulty = Difficulty::EASY;

    // Shuffle three labels (choices[0] must be the correct one before
    // calling this) and return the new correct index.
    static int ShuffleChoices(std::array<std::string, 3>& choices) {
        int order[3] = { 0, 1, 2 };
        std::shuffle(std::begin(order), std::end(order), RNG());
        std::array<std::string, 3> shuffled;
        int correctIndex = 0;
        for (int i = 0; i < 3; i++) {
            shuffled[i] = choices[order[i]];
            if (order[i] == 0) correctIndex = i;
        }
        choices = shuffled;
        return correctIndex;
    }
};

using PuzzlePtr = std::shared_ptr<Puzzle>;
using PuzzleFactory = std::function<PuzzlePtr()>;

inline void DrawCenteredText(const std::string& text, Rectangle bounds, int fontSize, Color color) {
    int w = MeasureText(text.c_str(), fontSize);
    while (w > (int)bounds.width && fontSize > 6) {
        fontSize--;
        w = MeasureText(text.c_str(), fontSize);
    }
    Vector2 pos{ bounds.x + bounds.width / 2.0f - w / 2.0f, bounds.y + (bounds.height - fontSize) / 2.0f };
    DrawText(text.c_str(), (int)pos.x, (int)pos.y, fontSize, color);
}
