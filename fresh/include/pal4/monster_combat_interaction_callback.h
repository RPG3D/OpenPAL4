#pragma once

#include "pal4/monster_combat_position_dispatcher.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

struct MonsterBehaviorManagerState;

struct MonsterCombatCandidate {
    bool active{};
    std::uint32_t pursuit_mode{};
    std::uint32_t source_address{};
    std::uint32_t text_address{};
    std::uint32_t timer_word{};
    std::array<float, 3> position{};
    std::array<float, 3> collision_position{};
};

class IMonsterCombatInteractionRuntime {
public:
    virtual ~IMonsterCombatInteractionRuntime() = default;
    [[nodiscard]] virtual MonsterCombatCandidate Candidate(
        std::uint32_t candidate_address) = 0;
    [[nodiscard]] virtual std::string ScriptFunction(
        std::uint32_t source_address) = 0;
    [[nodiscard]] virtual double TargetDoubleProperty(
        std::uint32_t target_address) = 0;
    [[nodiscard]] virtual double PlanarDistance(float dx, float dz) = 0;
    [[nodiscard]] virtual float InteractionRadius(
        std::uint32_t source_address) = 0;
    [[nodiscard]] virtual std::uint32_t CollisionClump() = 0;
    [[nodiscard]] virtual bool CollisionBlocked(
        const std::array<float, 6>& endpoints,
        std::uint32_t clump_address) = 0;
    [[nodiscard]] virtual IMonsterCombatPositionRuntime& Records() = 0;
};

class MonsterCombatInteractionRuntimeStub final
    : public IMonsterCombatInteractionRuntime {
public:
    MonsterCombatCandidate Candidate(std::uint32_t candidate_address) override;
    std::string ScriptFunction(std::uint32_t source_address) override;
    double TargetDoubleProperty(std::uint32_t target_address) override;
    double PlanarDistance(float dx, float dz) override;
    float InteractionRadius(std::uint32_t source_address) override;
    std::uint32_t CollisionClump() override;
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t clump_address) override;
    IMonsterCombatPositionRuntime& Records() override;

private:
    MonsterCombatPositionRuntimeStub records_{};
};

class MonsterCombatInteractionCallback final {
public:
    // IDA 0x434810: proximity-triggered combat-position handoff.
    [[nodiscard]] static std::uint32_t Process(
        MonsterBehaviorManagerState& manager_state,
        std::uint32_t candidate_address, std::uint32_t manager_address,
        IMonsterCombatInteractionRuntime& runtime);
};

}  // namespace pal4
