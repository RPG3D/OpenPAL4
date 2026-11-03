#pragma once

#include "pal4/game_data_parameter_record.h"

#include <cstdint>

namespace pal4 {

class IGameDataParameterRecordArrayRuntime
    : public IGameDataParameterRecordDestroyRuntime {
public:
    virtual ~IGameDataParameterRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source) = 0;
};

class GameDataParameterRecordArray final {
public:
    // IDA 0x4e3610: insert one 328-byte record, growing if full.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t at, std::uint32_t record,
        IGameDataParameterRecordArrayRuntime& runtime);
    // IDA 0x4e3820: count live records in the three-pointer array.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IGameDataParameterRecordArrayRuntime& runtime);
    // IDA 0x4e3850: copy-construct a half-open record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataParameterRecordArrayRuntime& runtime);
    // IDA 0x4e3890: insert repeated deep copies of a record.
    static void InsertCopies(std::uint32_t array, std::uint32_t at,
                             std::uint32_t count, std::uint32_t record,
                             IGameDataParameterRecordArrayRuntime& runtime);
    // IDA 0x4e3b90: construct count copies from one record.
    static void FillConstruct(std::uint32_t destination,
                              std::uint32_t count,
                              std::uint32_t record,
                              IGameDataParameterRecordArrayRuntime& runtime);
};

}  // namespace pal4
