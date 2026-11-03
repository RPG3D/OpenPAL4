#pragma once

#include "pal4/pal_movement_primary_child.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IPalMovementPrimaryCollisionRuntime {
public:
    virtual ~IPalMovementPrimaryCollisionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CheckObjectIntersection(
        std::uint32_t first, float second, float third,
        std::uint32_t fourth, std::uint32_t field70,
        std::uint32_t field6c) = 0;
    [[nodiscard]] virtual std::uint32_t CheckHeightIntersection(
        std::uint32_t source, float second, float third,
        std::array<float, 3>& output, std::uint32_t zero_one,
        std::uint32_t zero_two) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
};

class PalMovementPrimaryCollisionRuntimeStub final
    : public IPalMovementPrimaryCollisionRuntime {
public:
    std::uint32_t CheckObjectIntersection(
        std::uint32_t first, float second, float third,
        std::uint32_t fourth, std::uint32_t field70,
        std::uint32_t field6c) override;
    std::uint32_t CheckHeightIntersection(
        std::uint32_t source, float second, float third,
        std::array<float, 3>& output, std::uint32_t zero_one,
        std::uint32_t zero_two) override;
    float ReadFloat(std::uint32_t address) override;
};

}  // namespace pal4
