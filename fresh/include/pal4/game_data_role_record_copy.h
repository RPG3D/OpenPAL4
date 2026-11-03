#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IGameDataRoleRecordCopyRuntime {
public:
    virtual ~IGameDataRoleRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t AssignText(
        std::uint32_t destination, std::uint32_t source) = 0;
};

class GameDataRoleRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 320;

    // IDA 0x4ed890: assign one source record over a half-open range.
    [[nodiscard]] static std::uint32_t AssignRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source, IGameDataRoleRecordCopyRuntime& runtime);
    // IDA 0x4edb20: assign a range backward into an overlapping destination.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t source_end,
        std::uint32_t destination_end,
        IGameDataRoleRecordCopyRuntime& runtime);
    // IDA 0x4eddb0: construct one deep-copied record.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        IGameDataRoleRecordCopyRuntime& runtime);
    // IDA 0x4edf50: assign one Role record and return its destination.
    [[nodiscard]] static std::uint32_t AssignOne(
        std::uint32_t destination, std::uint32_t source,
        IGameDataRoleRecordCopyRuntime& runtime);
    // IDA 0x4ee1a0: assign the final 176 bytes of a Role record.
    [[nodiscard]] static std::uint32_t AssignTail(
        std::uint32_t destination, std::uint32_t source,
        IGameDataRoleRecordCopyRuntime& runtime);
    // IDA 0x4ee470: copy the first record and each later non-equivalent record.
    [[nodiscard]] static std::uint32_t UniqueCopy(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equivalent,
        IGameDataRoleRecordCopyRuntime& runtime);

private:
    static void AssignRecord(std::uint32_t destination,
                             std::uint32_t source,
                             IGameDataRoleRecordCopyRuntime& runtime);
};

}  // namespace pal4
