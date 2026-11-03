#pragma once

#include <cstdint>

namespace pal4 {

class IInventorySubsystemRuntime {
public:
    virtual ~IInventorySubsystemRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t name_address, std::uint32_t singleton_id,
        std::uint32_t factory_address,
        std::uint32_t cleanup_address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void InitializeBase(std::uint32_t object) = 0;
    virtual void InitializeSlots(std::uint32_t slots) = 0;
    virtual void InitializeArray(std::uint32_t array) = 0;
    virtual void CleanupArray(std::uint32_t array) = 0;
    virtual void CleanupSlots(std::uint32_t slots) = 0;
    virtual void CleanupBase(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GameDbInstance() = 0;
    [[nodiscard]] virtual bool HasFirst(std::uint32_t collection,
                                        std::uint32_t id) = 0;
    virtual void ProcessFirst(std::uint32_t collection,
                              std::uint32_t id) = 0;
    [[nodiscard]] virtual bool HasSecond(std::uint32_t database,
                                         std::uint32_t id) = 0;
    virtual void CalculateSecond(std::uint32_t database,
                                 std::uint32_t id) = 0;
    virtual void ProcessItem(std::uint32_t object,
                             std::uint32_t id, std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint8_t HasThird(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessThird(
        std::uint32_t collection, std::uint32_t id) = 0;
};

class IInventoryResetRuntime {
public:
    virtual ~IInventoryResetRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ResetSlots(std::uint32_t slots) = 0;
    virtual void ResetArray(std::uint32_t array) = 0;
    [[nodiscard]] virtual std::uint32_t ResetBase(
        std::uint32_t object) = 0;
};

class InventorySubsystem final {
public:
    static std::int32_t UpdatedCounter(std::uint32_t previous,std::int32_t delta);
    static constexpr std::uint32_t kNameAddress = 0x8b23f4u;
    static constexpr std::uint32_t kSingletonId = 1024u;
    static constexpr std::uint32_t kFactoryAddress = 0x4c5ec0u;
    static constexpr std::uint32_t kCleanupAddress = 0x4c5f20u;

    // IDA 0x4c5e80. guard and cached_instance represent global state.
    [[nodiscard]] static std::uint32_t InitializeSingleton(
        std::uint8_t& guard, std::uint32_t& cached_instance,
        IInventorySubsystemRuntime& runtime);
    // IDA 0x4c5ec0: allocate and construct a 312-byte instance.
    [[nodiscard]] static std::uint32_t Create(
        IInventorySubsystemRuntime& runtime);
    // IDA 0x4c5f20: cleanup and free nonnull instance.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IInventorySubsystemRuntime& runtime);
    // IDA 0x4c5f40 / 0x4c5fa0: embedded member lifecycle.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IInventorySubsystemRuntime& runtime);
    static void Cleanup(std::uint32_t object,
                        IInventorySubsystemRuntime& runtime);
    // IDA 0x4c6010: update the signed 32-bit counter and clamp it.
    [[nodiscard]] static std::int32_t UpdateCounter(
        std::uint32_t object, std::int32_t delta,
        IInventorySubsystemRuntime& runtime);
    // IDA 0x4c6050: initialize three GameDB-backed inventory ranges.
    [[nodiscard]] static std::uint8_t InitializeGameData(
        std::uint32_t object, IInventorySubsystemRuntime& runtime);
    // IDA 0x4c60f0: reset the counter and three embedded regions.
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t object, IInventoryResetRuntime& runtime);
};

}  // namespace pal4
