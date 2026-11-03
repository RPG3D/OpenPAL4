#pragma once

#include "pal4/game_math_tables.h"

#include <array>

namespace pal4 {

using GameVector3 = std::array<float, 3>;

class GameVectorMath final {
public:
    // IDA 0x668440, 0x668490 and 0x668530.
    static void Normalize(GameVector3& vector,
                          const GameMathTables& tables);
    static void NormalizeAndClamp(GameVector3& vector,
                                  const GameMathTables& tables);
    [[nodiscard]] static double ApproximateMagnitude(
        const GameVector3& vector, const GameMathTables& tables);
    // Same recovered square-root table, initialized once for shared callers.
    [[nodiscard]] static double ApproximateMagnitude(const GameVector3& vector);
    // IDA 0x668560 reads source float slots through offset +40.
    static void CalculateAngles(const std::array<float, 11>& basis,
                                GameVector3& angles,
                                const GameMathTables& tables);
};

}  // namespace pal4
