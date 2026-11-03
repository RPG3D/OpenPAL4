#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace pal4 {

struct ActorSkillActionSnapshot {
    std::array<std::uint8_t, 336> primary{};
    std::array<std::uint32_t, 84> conditions{};
    std::vector<std::uint8_t> dynamic_values;
    std::vector<std::uint64_t> dynamic_pairs;
    std::uint32_t tail_first = 0;
    std::uint32_t tail_second = 0;

    [[nodiscard]] std::uint32_t FollowupChance() const {
        return conditions[74];
    }
};

class IActorManagerSkillActionRuntime {
public:
    virtual ~IActorManagerSkillActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindSkill(std::uint32_t id) = 0;
    [[nodiscard]] virtual bool CheckRandom(std::uint32_t actor,
                                           std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentState(
        std::uint32_t actor) = 0;
    virtual void InvokeSpecial(std::uint32_t actor, std::uint32_t target,
                               std::uint32_t skill,
                               std::uint32_t index) = 0;
    [[nodiscard]] virtual ActorSkillActionSnapshot CaptureState(
        std::uint32_t object) = 0;
    virtual void ProcessSkillChain(
        std::uint32_t skill, ActorSkillActionSnapshot& target,
        ActorSkillActionSnapshot& actor) = 0;
    [[nodiscard]] virtual bool CheckRandomExtended(
        std::uint32_t actor, std::uint32_t threshold) = 0;
    virtual void InvokeFollowup(std::uint32_t actor,
                                std::uint32_t target,
                                std::uint32_t skill,
                                std::uint32_t threshold) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessComplexList(
        std::uint32_t actor, std::uint32_t skill,
        std::uint32_t target) = 0;
};

class ActorManagerSkillAction final {
public:
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t actor, std::uint32_t skill_id,
        std::uint32_t target,
        IActorManagerSkillActionRuntime& runtime);
};

}  // namespace pal4
