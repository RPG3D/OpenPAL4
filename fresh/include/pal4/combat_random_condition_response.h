#pragma once

#include <cstdint>

namespace pal4 {

class ICombatRandomConditionResponseRuntime {
public:
    virtual ~ICombatRandomConditionResponseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CheckCondition(std::uint32_t address,
                                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t ApplyEvent(
        std::uint32_t target, std::uint32_t context,
        std::uint32_t action, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t target, std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::int32_t AdjustStat(
        std::uint32_t target, std::uint32_t slot,
        std::int32_t delta) = 0;
    [[nodiscard]] virtual std::int32_t TriggerEvent(std::uint32_t target) = 0;
};

class CombatRandomConditionResponse final {
public:
    // IDA 0x5810b0; the second native argument is unused.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t action, std::uint32_t context,
        std::uint32_t target,
        ICombatRandomConditionResponseRuntime& runtime);
    // IDA 0x5811c0; the second native argument is unused.
    [[nodiscard]] static std::int32_t Trigger(
        std::uint32_t action, std::uint32_t target,
        ICombatRandomConditionResponseRuntime& runtime);
};

}  // namespace pal4
