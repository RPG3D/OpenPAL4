#pragma once

#include <cstdint>

namespace pal4 {

struct GameDataRange {
    std::int32_t index;
    std::int32_t maximum;
};

class IGameDataCollectionRuntime {
public:
    virtual ~IGameDataCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDataOwner() = 0;
    [[nodiscard]] virtual std::uint32_t RecordCount(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool ContainsId(std::uint32_t set,
                                           std::uint32_t id) = 0;
    virtual void InsertUniqueId(std::uint32_t set,
                                std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
};

class GameDataCollection final {
public:
    // IDA 0x430820. Reads a signed value at record +56+4*index.
    [[nodiscard]] static bool CheckValueRange(
        GameDataRange range, std::uint32_t record,
        IGameDataCollectionRuntime& runtime);
    // IDA 0x430640. Returns output and writes its 16-byte pointer-array header.
    [[nodiscard]] static std::uint32_t BuildNewMatches(
        std::uint32_t set, std::uint32_t output, GameDataRange range,
        std::uint32_t seed_address, IGameDataCollectionRuntime& runtime);
};

}  // namespace pal4
