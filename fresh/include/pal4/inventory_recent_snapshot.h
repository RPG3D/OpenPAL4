#pragma once

#include <cstdint>

namespace pal4 {

class IInventoryRecentSnapshotRuntime {
public:
    virtual ~IInventoryRecentSnapshotRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class InventoryRecentSnapshot final {
public:
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t inventory, std::uint32_t destination,
        IInventoryRecentSnapshotRuntime& runtime);
};

}  // namespace pal4
