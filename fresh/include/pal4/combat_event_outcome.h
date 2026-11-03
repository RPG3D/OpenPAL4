#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventOutcomeRuntime {
public:
    virtual ~ICombatEventOutcomeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    virtual void SetEventData(std::uint32_t slot, std::uint32_t target,
                              std::int32_t first, float scale,
                              std::int32_t second) = 0;
    virtual void ProcessObject(std::uint32_t slot,
                               std::uint32_t target) = 0;
    [[nodiscard]] virtual bool Matches(std::uint32_t slot,
                                       std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::uint32_t EnumerateTargets(
        std::uint32_t target, std::uint8_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorBegin(
        std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorEnd(
        std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::uint32_t CandidateStatus(
        std::uint32_t candidate) = 0;
    [[nodiscard]] virtual std::uint32_t FreeVector(
        std::uint32_t vector) = 0;
};

class ICombatMultiEffectRuntime : public ICombatEventOutcomeRuntime {
public:
    [[nodiscard]] virtual bool CheckCandidateActive(
        std::uint32_t candidate) = 0;
    virtual void DispatchPair(std::uint32_t source, std::int32_t first,
                              std::int32_t second) = 0;
};

class CombatEventOutcome final {
public:
    // IDA 0x57fea0. The second and third native arguments are unused.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t record, std::uint32_t unused_second,
        std::uint32_t unused_third, std::uint32_t target,
        ICombatEventOutcomeRuntime& runtime);
    // IDA 0x5803d0.
    [[nodiscard]] static std::int32_t ProcessMultiTarget(
        std::uint32_t action, std::uint32_t target,
        ICombatEventOutcomeRuntime& runtime);
    // IDA 0x580590. The register argument in the native signature is unused.
    [[nodiscard]] static std::int32_t ProcessMultiEffect(
        std::uint32_t unused_register, std::uint32_t action,
        std::uint32_t source, std::uint32_t target,
        ICombatMultiEffectRuntime& runtime);
};

}  // namespace pal4
