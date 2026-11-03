#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopGoodsRecordStorageRuntime {
public:
    virtual ~IGameDataShopGoodsRecordStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffectData(
        std::uint32_t object) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataShopGoodsRecordStorage final {
public:
    static constexpr std::uint32_t kRecordBytes = 36;

    // IDA 0x4eff20: destroy and release an array of Good records.
    [[nodiscard]] static std::uint32_t ClearAndFree(
        std::uint32_t array, IGameDataShopGoodsRecordStorageRuntime& runtime);
    // IDA 0x4eff60: clear an object's active Good records, retaining capacity.
    [[nodiscard]] static std::uint32_t ClearObjectRecords(
        std::uint32_t object, IGameDataShopGoodsRecordStorageRuntime& runtime);
    // IDA 0x4f0630: destroy a half-open 36-byte record range.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t incoming_eax,
        IGameDataShopGoodsRecordStorageRuntime& runtime);
};

}  // namespace pal4
