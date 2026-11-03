#pragma once

#include <cstdint>

namespace pal4 {

class ICombatProbabilityOutcomeRuntime {
public:
    virtual ~ICombatProbabilityOutcomeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t ApplyOutcome(
        std::uint32_t target, std::int32_t value,
        std::uint32_t context) = 0;
};

class CombatProbabilityOutcome final {
public:
    // IDA 0x57fd80; the second native argument is unused.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t record, std::uint32_t unused,
        std::uint32_t context, std::uint32_t target,
        ICombatProbabilityOutcomeRuntime& runtime);
};

class ICombatProbabilityOutcomeExtendedRuntime {
public:
    virtual ~ICombatProbabilityOutcomeExtendedRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t LookupEvent(std::int32_t index) = 0;
    [[nodiscard]] virtual float QueryEventValue(
        std::uint32_t event, std::uint32_t actor_data) = 0;
    [[nodiscard]] virtual std::int32_t ApplyOutcome(
        std::uint32_t target, std::int32_t value,
        std::uint32_t actor) = 0;
};

class CombatProbabilityOutcomeExtended final {
public:
    // IDA 0x57fdf0; the second native argument is unused.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t record, std::uint32_t unused,
        std::uint32_t target, std::uint32_t actor,
        ICombatProbabilityOutcomeExtendedRuntime& runtime);
};

}  // namespace pal4
