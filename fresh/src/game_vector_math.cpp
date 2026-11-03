#include "pal4/game_vector_math.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float LengthSquared(const GameVector3& vector) {
    // IDA668450..66845C / 66853F..66854B: the game runs x87 PC24.
    // Round each product and each addition before the table lookup.
    const float x_squared = vector[0] * vector[0];
    const float y_squared = vector[1] * vector[1];
    const float z_squared = vector[2] * vector[2];
    const float xy_squared = x_squared + y_squared;
    return xy_squared + z_squared;
}

}  // namespace

void GameVectorMath::Normalize(GameVector3& vector,
                               const GameMathTables& tables) {
    const auto scale = tables.ApproximateInverseSquareRoot(
        LengthSquared(vector));
    for (auto& component : vector)
        component = static_cast<float>(scale * component);
}

void GameVectorMath::NormalizeAndClamp(GameVector3& vector,
                                       const GameMathTables& tables) {
    Normalize(vector, tables);
    for (auto& component : vector) {
        if (component < -1.0f) component = -1.0f;
        else if (component >= 1.0f) component = 1.0f;
    }
}

double GameVectorMath::ApproximateMagnitude(const GameVector3& vector) {
    static const GameMathTables tables=[] {
        GameMathTables value;(void)value.CalculateSquareRootTable();return value;
    }();
    return ApproximateMagnitude(vector,tables);
}

double GameVectorMath::ApproximateMagnitude(
    const GameVector3& vector, const GameMathTables& tables) {
    return tables.ApproximateSquareRoot(LengthSquared(vector));
}

void GameVectorMath::CalculateAngles(
    const std::array<float, 11>& basis, GameVector3& angles,
    const GameMathTables& tables) {
    constexpr auto degrees = std::bit_cast<float>(0x42652ee1u);
    constexpr auto threshold =
        std::bit_cast<double>(0x3f40624de0000000ull);
    const auto pitch = static_cast<float>(
        -tables.ApproximateArcsine(basis[2]));
    const auto cosine = tables.ApproximateCosine(pitch);
    angles[1] = static_cast<float>(degrees * pitch);

    if (std::abs(cosine) > threshold) {
        const auto inverse = 1.0 / cosine;
        angles[0] = static_cast<float>(std::atan2(
            inverse * basis[6], inverse * basis[10]) * degrees);
        angles[2] = static_cast<float>(std::atan2(
            inverse * basis[1], inverse * basis[0]) * degrees);
    } else {
        angles[0] = 0.0f;
        angles[2] = static_cast<float>(
            std::atan2(-basis[4], basis[5]) * degrees);
    }
    for (auto& angle : angles) {
        if (angle < 0.0f) angle += 360.0f;
    }
}

}  // namespace pal4
