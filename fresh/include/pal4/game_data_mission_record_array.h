#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataMissionRecordArrayRuntime {
public:
    virtual ~IGameDataMissionRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source) = 0;
    virtual void Destroy(std::uint32_t record) = 0;
};

class GameDataMissionRecordArray final {
public:
    // IDA 0x4dc2a0: insert a deep-copied 68-byte record at a raw array slot.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t at, std::uint32_t record,
        IGameDataMissionRecordArrayRuntime& runtime);
    // IDA 0x4dc490: copy-construct a half-open range of 68-byte records.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataMissionRecordArrayRuntime& runtime);
    // IDA 0x4dc4d0: insert repeated deep copies of one 68-byte record.
    static void InsertCopies(std::uint32_t array, std::uint32_t at,
                             std::uint32_t count, std::uint32_t record,
                             IGameDataMissionRecordArrayRuntime& runtime);
    // IDA 0x4dc7f0: copy-construct count records from one template.
    static void FillConstruct(std::uint32_t destination,
                              std::uint32_t count, std::uint32_t record,
                              IGameDataMissionRecordArrayRuntime& runtime);
    // IDA 0x4dca50: assign one record to every existing slot in a range.
    [[nodiscard]] static std::uint32_t AssignCopies(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t record,
        IGameDataMissionRecordArrayRuntime& runtime);
    // IDA 0x4dcc80: assign records backward into an overlapping range.
    [[nodiscard]] static std::uint32_t ShiftBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameDataMissionRecordArrayRuntime& runtime);
};

}  // namespace pal4
