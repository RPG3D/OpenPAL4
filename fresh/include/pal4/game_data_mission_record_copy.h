#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataMissionRecordCopyRuntime {
public:
    virtual ~IGameDataMissionRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AssignText(
        std::uint32_t destination, std::uint32_t source) = 0;
};

class GameDataMissionRecordCopy final {
public:
    // IDA 0x4dcee0: copy-construct five scalar words and three text records.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t destination, std::uint32_t source,
        IGameDataMissionRecordCopyRuntime& runtime);
    // IDA 0x4dcfe0: assign five scalar words and three existing text records.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameDataMissionRecordCopyRuntime& runtime);
};

}  // namespace pal4
