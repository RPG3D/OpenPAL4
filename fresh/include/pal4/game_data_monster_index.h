#pragma once

#include "pal4/game_data_monster_record.h"

#include <cstdint>

namespace pal4 {

class IGameDataMonsterIndexRuntime
    : public IGameDataMonsterRecordDestroyRuntime {
public:
    virtual ~IGameDataMonsterIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class GameDataMonsterIndex final {
public:
    // IDA 0x4df3f0: compact adjacent duplicate records and rebuild index.
    static void Normalize(std::uint32_t table,
                          IGameDataMonsterIndexRuntime& runtime);
    // IDA 0x4df640: destroy 596-byte records over a half-open range.
    static void DestroyRange(std::uint32_t begin, std::uint32_t end,
                             IGameDataMonsterIndexRuntime& runtime);
    // IDA 0x4dfad0: copy first and adjacent-unique records to destination.
    [[nodiscard]] static std::uint32_t CompactFrom(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IGameDataMonsterIndexRuntime& runtime);
    // IDA 0x4df870: compare record keys at +400 for equality.
    [[nodiscard]] static bool EqualKeys(
        std::uint32_t left, std::uint32_t right,
        IGameDataMonsterIndexRuntime& runtime);
};

}  // namespace pal4
