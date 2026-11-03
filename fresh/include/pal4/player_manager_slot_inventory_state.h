#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerSlotInventoryRuntime {
public:
    virtual ~IPlayerManagerSlotInventoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindItem(
        std::uint32_t collection, std::uint32_t key,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint8_t CheckPlayerCondition(
        std::uint32_t item) = 0;
};

class PlayerManagerSlotInventoryRuntimeStub :
    public IPlayerManagerSlotInventoryRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    std::uint32_t InventoryManager() override;
    std::uint32_t FindItem(std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    std::uint8_t CheckPlayerCondition(std::uint32_t) override;
};

class PlayerManagerSlotInventoryState final {
public:
    // IDA 0x43f410: validate six equipment slots on activation.
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, std::uint32_t enabled,
        IPlayerManagerSlotInventoryRuntime& runtime);
};

}  // namespace pal4
