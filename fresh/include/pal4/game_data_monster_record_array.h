#pragma once

#include "pal4/game_data_monster_record.h"

#include <cstdint>

namespace pal4 {

class IGameDataMonsterRecordArrayRuntime
    : public IGameDataMonsterRecordDestroyRuntime {
public:
    virtual ~IGameDataMonsterRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source) = 0;
};

class GameDataMonsterRecordArray final {
public:
    // IDA 0x4df890: insert one deep-copied 596-byte record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t at, std::uint32_t record,
        IGameDataMonsterRecordArrayRuntime& runtime);
    // IDA 0x4dfaa0: count live records in a three-pointer array.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IGameDataMonsterRecordArrayRuntime& runtime);
    // IDA 0x4dfad0: copy-construct a half-open range of records.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataMonsterRecordArrayRuntime& runtime);
    // IDA 0x4dfb10: insert repeated deep copies of one Monster record.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t at,
        std::uint32_t count, std::uint32_t record,
        IGameDataMonsterRecordArrayRuntime& runtime);
    // IDA 0x4dfe60: construct count consecutive records from a template.
    static void FillConstruct(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t record,
        IGameDataMonsterRecordArrayRuntime& runtime);
};

}  // namespace pal4
