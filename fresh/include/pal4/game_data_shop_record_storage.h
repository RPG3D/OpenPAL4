#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataShopRecordStorageRuntime {
public:
    virtual ~IGameDataShopRecordStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffectData(
        std::uint32_t object) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class GameDataShopRecordStorage final {
public:
    static constexpr std::uint32_t kRecordBytes = 56;

    // IDA 0x4ef3c0: destroy a vector's records, free storage and clear it.
    [[nodiscard]] static std::uint32_t ClearAndFree(
        std::uint32_t array, IGameDataShopRecordStorageRuntime& runtime);
    // IDA 0x4ef400: clear Effect state and destroy the object's active records.
    [[nodiscard]] static std::uint32_t ClearObjectRecords(
        std::uint32_t object, IGameDataShopRecordStorageRuntime& runtime);
    // IDA 0x4efb70: destroy a half-open range in 56-byte steps.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t incoming_eax,
        IGameDataShopRecordStorageRuntime& runtime);
};

}  // namespace pal4
