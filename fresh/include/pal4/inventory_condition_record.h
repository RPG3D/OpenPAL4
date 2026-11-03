#pragma once

#include <cstdint>

namespace pal4 {

class IInventoryConditionStreamRuntime {
public:
    virtual ~IInventoryConditionStreamRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteStreamWord(std::uint32_t stream, std::uint32_t address,
                                 std::uint32_t bytes) = 0;
    virtual void ReadStreamWord(std::uint32_t stream, std::uint32_t address,
                                std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::int32_t SerializeRecord(std::uint32_t record,
                                                        std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::int32_t DeserializeRecord(std::uint32_t record,
                                                          std::uint32_t stream) = 0;
};

class IInventoryConditionRuntime : public IInventoryConditionStreamRuntime {
public:
    [[nodiscard]] virtual std::uint32_t InitializePlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t GetPlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool InventoryContains(std::uint32_t slots,
                                                  std::uint32_t item) = 0;
};

class InventoryConditionRecord final {
public:
    // IDA 0x4c41b0/0x4c41e0. The stream helper receives a 4-byte field at +0x190.
    [[nodiscard]] static std::int32_t Serialize(std::uint32_t record,
                                                 std::uint32_t stream,
                                                 IInventoryConditionStreamRuntime& runtime);
    [[nodiscard]] static std::int32_t Deserialize(std::uint32_t record,
                                                   std::uint32_t stream,
                                                   IInventoryConditionStreamRuntime& runtime);
    // IDA 0x4c4210/0x4c4260. Player status is at +0xb08, slots at +0x920.
    [[nodiscard]] static bool AnyActivePlayerHasItem(
        std::uint32_t record, IInventoryConditionRuntime& runtime);
    [[nodiscard]] static bool NoInactivePlayerHasItem(
        std::uint32_t record, IInventoryConditionRuntime& runtime);
};

}  // namespace pal4
