#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActionFollowupsRuntime {
public:
    virtual ~ICombatActionFollowupsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::uint32_t EnumerateTargets(
        std::uint32_t actor, std::uint8_t selector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorBegin(std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::uint32_t VectorEnd(std::uint32_t vector) = 0;
    virtual void ResetSlot(std::uint32_t actor,
                           std::uint32_t slot,
                           std::int32_t value) = 0;
    virtual void RefreshActor(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t FreeVector(std::uint32_t vector) = 0;
    [[nodiscard]] virtual std::int32_t TransferValue(
        std::uint32_t actor, std::int32_t value) = 0;
    virtual void CleanupCondition(std::uint32_t actor,
                                  std::int32_t id,
                                  std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t NotifyCleanup(
        std::uint32_t actor) = 0;
};

class CombatActionFollowups final {
public:
    // IDA 0x580ca0.
    [[nodiscard]] static std::int32_t ResetOtherTargets(
        std::uint32_t actor, ICombatActionFollowupsRuntime& runtime);
    // IDA 0x580d40.
    [[nodiscard]] static std::int32_t Transfer(
        std::uint32_t action, std::uint32_t target,
        std::uint32_t source, ICombatActionFollowupsRuntime& runtime);
    // IDA 0x580d70.
    [[nodiscard]] static std::uint8_t CleanupConditions(
        std::uint32_t action, std::uint32_t actor,
        ICombatActionFollowupsRuntime& runtime);
};

}  // namespace pal4
