#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

class ICombatConditionalActionsRuntime {
public:
    virtual ~ICombatConditionalActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(std::uint32_t actor,
                                                  std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::int32_t ActorBase(std::uint32_t actor,
                                                  std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::int32_t ActorCounter(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    virtual void ApplySelected(std::uint32_t target, std::int32_t id) = 0;
    [[nodiscard]] virtual std::int32_t ReleaseIds(
        const std::vector<std::int32_t>& ids) = 0;
    [[nodiscard]] virtual std::uint32_t EnumerateCandidates(
        std::uint32_t target, std::uint8_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorBegin(std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorEnd(std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::int32_t CandidateId(std::uint32_t candidate) = 0;
    virtual void ResetCandidate(std::uint32_t candidate,
                                std::uint32_t slot, std::int32_t value) = 0;
    virtual void RefreshCandidate(std::uint32_t candidate) = 0;
    virtual void FreeVector(std::uint32_t vector) = 0;
};

class CombatConditionalActions final {
public:
    // IDA 0x580dc0.
    [[nodiscard]] static std::int32_t Select(
        std::uint32_t action, std::uint32_t actor, std::uint32_t target,
        ICombatConditionalActionsRuntime& runtime);
    // IDA 0x580f20.
    [[nodiscard]] static std::int32_t ResetMatching(
        std::uint32_t action, std::uint32_t actor, std::uint32_t target,
        ICombatConditionalActionsRuntime& runtime);
};

}  // namespace pal4
