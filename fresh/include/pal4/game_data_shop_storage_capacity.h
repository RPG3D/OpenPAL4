#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopStorageCapacityRuntime {
public:
    virtual ~IGameDataShopStorageCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t address) = 0;
    virtual void CopyIndex(std::uint32_t destination,
                           std::uint32_t source) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataShopStorageCapacity final {
public:
    static constexpr std::uint32_t kRecordBytes = 56;
    static constexpr std::uint32_t kIndexBytes = 8;

    // IDA 0x4efba0: reserve both owned arrays, preserving their active sizes.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IGameDataShopStorageCapacityRuntime& runtime);
    // IDA 0x4f0660: same two-array reserve with 36-byte Good records.
    [[nodiscard]] static std::uint32_t ReserveGoods(
        std::uint32_t object, std::int32_t requested,
        IGameDataShopStorageCapacityRuntime& runtime);
};

}  // namespace pal4
