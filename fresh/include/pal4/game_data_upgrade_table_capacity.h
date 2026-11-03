#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataUpgradeTableCapacityRuntime {
public:
    virtual ~IGameDataUpgradeTableCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void CopyIndex(std::uint32_t destination,
                           std::uint32_t source) = 0;
};

class GameDataUpgradeTableCapacity final {
public:
    // IDA 0x4eb510: independently grow 160-byte records and 8-byte keys.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IGameDataUpgradeTableCapacityRuntime& runtime);
};

}  // namespace pal4
