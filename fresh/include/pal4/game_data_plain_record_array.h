#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IGameDataPlainRecordArrayRuntime {
public:
    virtual ~IGameDataPlainRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t bytes) = 0;
};

class GameDataUpgradeRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 160;
    // IDA 0x4ec380, 0x4ec5a0, 0x4ec5d0.
    [[nodiscard]] static std::uint32_t InsertOne(
        std::uint32_t array, std::uint32_t position, std::uint32_t source,
        IGameDataPlainRecordArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t Count(
        std::uint32_t array, IGameDataPlainRecordArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4ecb80: insert repeated 160-byte records.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4ed180: fill a range with copies of a 160-byte record.
    [[nodiscard]] static std::uint32_t FillRange(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source,
        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4edec0: copy one record when its destination exists.
    static void CopyOne(std::uint32_t destination, std::uint32_t source,
                        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4ee740: retain the first record of each adjacent equivalent run.
    [[nodiscard]] static std::uint32_t UniqueCopy(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
        IGameDataPlainRecordArrayRuntime& runtime);
};

class GameDataCombatPrizeRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 128;
    // IDA 0x4ec610, 0x4ec7d0, 0x4ec7f0.
    [[nodiscard]] static std::uint32_t InsertOne(
        std::uint32_t array, std::uint32_t position, std::uint32_t source,
        IGameDataPlainRecordArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t Count(
        std::uint32_t array, IGameDataPlainRecordArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4ece90: insert repeated 128-byte records.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4ed1c0: fill a range with copies of a 128-byte record.
    [[nodiscard]] static std::uint32_t FillRange(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source,
        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4edee0: copy one record when its destination exists.
    static void CopyOne(std::uint32_t destination, std::uint32_t source,
                        IGameDataPlainRecordArrayRuntime& runtime);
    // IDA 0x4ee7c0: retain the first record of each adjacent equivalent run.
    [[nodiscard]] static std::uint32_t UniqueCopy(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
        IGameDataPlainRecordArrayRuntime& runtime);
};

}  // namespace pal4
