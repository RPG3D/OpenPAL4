#pragma once

#include <cstdint>

namespace pal4 {

class ICombatScalarEffectsRuntime {
public:
    virtual ~ICombatScalarEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int64_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t SetProperty(std::uint32_t target,
        std::uint32_t kind, std::uint32_t value) = 0;
};

class ICombatIndexedEffectsRuntime {
public:
    virtual ~ICombatIndexedEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int64_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::uint32_t LookupEvent(std::int32_t index) = 0;
    [[nodiscard]] virtual std::int32_t DispatchEvent(std::uint32_t event,
        std::uint32_t target,std::uint32_t bits) = 0;
};

class ICombatStatTransferRuntime {
public:
    virtual ~ICombatStatTransferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int64_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual bool CheckFurther(std::uint32_t context,
                                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t DispatchAction(
        std::uint32_t target, std::uint32_t context,
        std::uint32_t action, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t GetStat(std::uint32_t actor,
                                               std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t AddStat(std::uint32_t actor,
                                               std::uint32_t stat,
                                               std::int64_t delta) = 0;
};

class ICombatSlotConsumeRuntime {
public:
    virtual ~ICombatSlotConsumeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t GetSlot(std::uint32_t target,std::uint32_t slot) = 0;
    virtual void SetSlot(std::uint32_t target,std::uint32_t slot,std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t TransferSlots(std::uint32_t source,std::int32_t first,std::int32_t second) = 0;
    [[nodiscard]] virtual std::int32_t TransferPositiveSlot(std::uint32_t source,std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t Fallback(std::uint32_t source) = 0;
};

class ICombatActionEffectsRuntime : public ICombatScalarEffectsRuntime,
                                   public ICombatStatTransferRuntime,
                                   public ICombatIndexedEffectsRuntime,
                                   public ICombatSlotConsumeRuntime {
public:
    virtual ~ICombatActionEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int64_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual bool CheckFurther(std::uint32_t context,
                                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t DispatchAction(
        std::uint32_t target, std::uint32_t context,
        std::uint32_t action, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t GetStat(std::uint32_t actor,
                                               std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t AddStat(std::uint32_t actor,
                                               std::uint32_t stat,
                                               std::int64_t delta) = 0;
    [[nodiscard]] virtual std::int32_t GetSlot(std::uint32_t target,
                                               std::uint32_t slot) = 0;
    virtual void SetSlot(std::uint32_t target, std::uint32_t slot,
                         std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t TransferSlots(
        std::uint32_t source, std::int32_t first,
        std::int32_t second) = 0;
    [[nodiscard]] virtual std::int32_t TransferPositiveSlot(
        std::uint32_t source, std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t Fallback(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::int32_t SetProperty(
        std::uint32_t target, std::uint32_t kind,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t LookupEvent(
        std::int32_t index) = 0;
    [[nodiscard]] virtual std::int32_t DispatchEvent(
        std::uint32_t event, std::uint32_t target,
        std::uint32_t value) = 0;
};

class CombatActionEffects final {
public:
    // IDA 0x5800b0.
    [[nodiscard]] static std::int32_t HandlePropertyType(
        std::uint32_t action, std::uint32_t target,
        ICombatScalarEffectsRuntime& runtime);
    // IDA 0x5800f0.
    [[nodiscard]] static std::int32_t ProcessStatTransfer(
        std::uint32_t action, std::uint32_t source,
        std::uint32_t context, std::uint32_t target,
        ICombatStatTransferRuntime& runtime);
    // IDA 0x5801f0.
    [[nodiscard]] static std::int32_t ConsumeSlotsZeroOne(
        std::uint32_t action, std::uint32_t source,
        std::uint32_t target, ICombatSlotConsumeRuntime& runtime);
    // IDA 0x5802d0.
    [[nodiscard]] static std::int32_t ConsumeSlotTwo(
        std::uint32_t action, std::uint32_t source,
        std::uint32_t target, ICombatSlotConsumeRuntime& runtime);
    // IDA 0x580390.
    [[nodiscard]] static std::int32_t HandlePropertyTypeFour(
        std::uint32_t action, std::uint32_t target,
        ICombatScalarEffectsRuntime& runtime);
    // IDA 0x5803b0.
    [[nodiscard]] static std::int32_t HandleStatDeltaThree(
        std::uint32_t action, std::uint32_t target,
        ICombatActionEffectsRuntime& runtime);
    // IDA 0x580510/0x580530/0x580550.
    [[nodiscard]] static std::int32_t HandlePropertyTypeZero(
        std::uint32_t action, std::uint32_t target,
        ICombatScalarEffectsRuntime& runtime);
    [[nodiscard]] static std::int32_t HandlePropertyTypeOne(
        std::uint32_t action, std::uint32_t target,
        ICombatScalarEffectsRuntime& runtime);
    // Shared native callback selection for Player, Monster and action snapshots.
    // Returns false without dispatch for callbacks outside the four scalar entries.
    [[nodiscard]] static bool TryScalarCallback(std::uint32_t callback,
        std::uint32_t action,std::uint32_t target,ICombatScalarEffectsRuntime& runtime);
    [[nodiscard]] static std::int32_t DispatchIndexed(
        std::uint32_t action, std::uint32_t target,
        ICombatIndexedEffectsRuntime& runtime);
};

}  // namespace pal4
