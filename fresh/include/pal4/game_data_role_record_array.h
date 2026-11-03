#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRoleRecordArrayRuntime {
public:
    virtual ~IGameDataRoleRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void FillConstruct(std::uint32_t destination,
                               std::uint32_t count,
                               std::uint32_t source) = 0;
    virtual void AssignBackward(std::uint32_t begin,
                                std::uint32_t source_end,
                                std::uint32_t destination_end) = 0;
    virtual void AssignRange(std::uint32_t begin,
                             std::uint32_t end,
                             std::uint32_t source) = 0;
    virtual void DestroyRange(std::uint32_t begin,
                              std::uint32_t end) = 0;
};

class GameDataRoleRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 320;

    // IDA 0x4ec110: insert one deep-copied Role record.
    [[nodiscard]] static std::uint32_t InsertOne(
        std::uint32_t array, std::uint32_t position, std::uint32_t source,
        IGameDataRoleRecordArrayRuntime& runtime);
    // IDA 0x4ec310: count live records in the array at object +4.
    [[nodiscard]] static std::uint32_t Count(
        std::uint32_t array, IGameDataRoleRecordArrayRuntime& runtime);
    // IDA 0x4ec340: copy-construct a half-open record range.
    [[nodiscard]] static std::uint32_t CopyConstructRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataRoleRecordArrayRuntime& runtime);
    // IDA 0x4ec830: insert repeated deep-copied Role records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataRoleRecordArrayRuntime& runtime);
    // IDA 0x4ed150: repeatedly copy-construct one Role record.
    static void FillConstructRange(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source,
        IGameDataRoleRecordArrayRuntime& runtime);
};

}  // namespace pal4
