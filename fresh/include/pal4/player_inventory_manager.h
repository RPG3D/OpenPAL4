#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerInventoryRuntime {
public:
    virtual ~IPlayerInventoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t collection, std::uint32_t key,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t GetPlayer(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool Contains(std::uint32_t slots,
                                         std::uint32_t item) = 0;
    [[nodiscard]] virtual std::int32_t FindSlot(
        std::uint32_t slots, std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t component, std::uint32_t vtable_slot) = 0;
};

class PlayerInventoryManager final {
public:
    // IDA 0x590ae0.
    static std::uint8_t Manage(std::uint32_t item,
                               std::uint32_t inventory_key,
                               IPlayerInventoryRuntime& runtime);
};

}  // namespace pal4
