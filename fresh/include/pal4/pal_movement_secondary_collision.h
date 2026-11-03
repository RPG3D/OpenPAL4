#pragma once

#include "pal4/pal_movement_secondary_child.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct PalMovementCollisionCandidate {
    std::uint32_t field80 = 0u;
    std::uint32_t field84 = 0u;
    std::uint32_t other_one = 0u;
    std::uint32_t other_two = 0u;
    float height = 0.0f;
};
static_assert(sizeof(PalMovementCollisionCandidate) == 0x14u);

class IPalMovementSecondaryCollisionRuntime {
public:
    virtual ~IPalMovementSecondaryCollisionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadElement(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetClump(
        std::uint32_t element) = 0;
    [[nodiscard]] virtual bool QueryCollisionData(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual bool ProcessClumpCollision(
        const std::array<float, 6>& line, std::uint32_t clump,
        PalMovementCollisionCandidate& candidate) = 0;
    [[nodiscard]] virtual double SwitchThreshold(std::uint32_t element) = 0;
    virtual void Leave(std::uint32_t element) = 0;
    virtual void Enter(std::uint32_t element) = 0;
};

class PalMovementSecondaryCollisionRuntimeStub final
    : public IPalMovementSecondaryCollisionRuntime {
public:
    std::uint32_t ReadElement(std::uint32_t address) override;
    std::uint32_t GetClump(std::uint32_t element) override;
    bool QueryCollisionData(std::uint32_t clump) override;
    bool ProcessClumpCollision(
        const std::array<float, 6>& line, std::uint32_t clump,
        PalMovementCollisionCandidate& candidate) override;
    double SwitchThreshold(std::uint32_t element) override;
    void Leave(std::uint32_t element) override;
    void Enter(std::uint32_t element) override;
};

}  // namespace pal4
