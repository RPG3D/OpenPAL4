#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataPropertyRecordCopyRuntime {
public:
    virtual ~IGameDataPropertyRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ConstructText(std::uint32_t destination,
                               std::uint32_t source) = 0;
};

class GameDataPropertyRecordCopy final {
public:
    // IDA 0x4e6c70: assign one source record across a destination range.
    [[nodiscard]] static std::uint32_t AssignRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source,
        IGameDataPropertyRecordCopyRuntime& runtime);
    // IDA 0x4e6e80: assign a source range backward into destination end.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination_end,
        IGameDataPropertyRecordCopyRuntime& runtime);
    // IDA 0x4e7070: copy-construct one record, including seven texts.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t destination, std::uint32_t source,
        IGameDataPropertyRecordCopyRuntime& runtime);
    // IDA 0x4e72a0: assign one complete Property record.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameDataPropertyRecordCopyRuntime& runtime);
    // IDA 0x4e7480: retain one record per adjacent equal-key run.
    [[nodiscard]] static std::uint32_t UniqueCopy(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataPropertyRecordCopyRuntime& runtime);
};

}  // namespace pal4
