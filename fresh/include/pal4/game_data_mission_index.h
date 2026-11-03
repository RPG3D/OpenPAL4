#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataMissionIndexRuntime {
public:
    virtual ~IGameDataMissionIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class GameDataMissionIndex final {
public:
    // IDA 0x4dbeb0: compact adjacent duplicate records and rebuild key index.
    static void Normalize(std::uint32_t table,
                          IGameDataMissionIndexRuntime& runtime);
    // IDA 0x4dc0f0: destroy 68-byte records over a half-open range.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IGameDataMissionIndexRuntime& runtime);
    // IDA 0x4dd240: copy the first record and later adjacent-unique records.
    [[nodiscard]] static std::uint32_t CompactFrom(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataMissionIndexRuntime& runtime);
};

}  // namespace pal4
