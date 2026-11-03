#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActorPropertyRuntime {
public:
    virtual ~ICombatActorPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ActorVirtual48(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ActorVirtual52(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ActorVirtual68(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t ActorVirtual84(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t OwnerVirtual168(
        std::uint32_t owner, std::uint32_t actor,
        std::uint32_t index, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t SharesOwner(
        std::uint32_t actor, std::uint32_t other) = 0;
    [[nodiscard]] virtual std::uint32_t ActorVirtual268(
        std::uint32_t actor, std::uint32_t code,
        std::uint32_t source) = 0;
};

class CombatActorPropertyCallbacks final {
public:
    // IDA 0x56bd30: adjust decreases to property slot 9.
    [[nodiscard]] static std::int32_t AdjustSlotNineDecrease(
        std::uint32_t callback, std::uint32_t actor,
        std::uint32_t index, std::int32_t value,
        ICombatActorPropertyRuntime& runtime);
    // IDA 0x56bda0: apply five percent of slot 2 to slot 9.
    [[nodiscard]] static std::int32_t ApplySlotNinePercent(
        std::uint32_t actor, ICombatActorPropertyRuntime& runtime);
    // IDA 0x56be10: owner check and actor virtual +268 notification.
    [[nodiscard]] static std::uint8_t NotifyDifferentOwner(
        std::uint32_t actor, std::uint32_t other,
        ICombatActorPropertyRuntime& runtime);
    // IDA 0x56be40: apply five percent of slot 0 to slot 7.
    [[nodiscard]] static std::int32_t ApplySlotSevenPercent(
        std::uint32_t actor, ICombatActorPropertyRuntime& runtime);
    // IDA 0x56be90: reflect slot-7 excess around its current value.
    [[nodiscard]] static std::int32_t ReflectSlotSevenExcess(
        std::uint32_t callback, std::uint32_t actor,
        std::uint32_t index, std::int32_t value,
        ICombatActorPropertyRuntime& runtime);
    // IDA 0x56bee0: scale slot-7/9 excess by the native -0.7 constant.
    [[nodiscard]] static std::int32_t ScaleSlotSevenNineExcess(
        std::uint32_t callback, std::uint32_t actor,
        std::uint32_t index, std::int32_t value,
        ICombatActorPropertyRuntime& runtime);
};

}  // namespace pal4
