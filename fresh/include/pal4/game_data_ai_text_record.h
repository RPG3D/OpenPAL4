#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataAiTextRecordRuntime {
public:
    virtual ~IGameDataAiTextRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class GameDataAiTextRecord final {
public:
    // IDA 0x4cd5b0: release three text references and zero their metadata.
    [[nodiscard]] static std::uint32_t ResetPointers(
        std::uint32_t record, IGameDataAiTextRecordRuntime& runtime);
    // IDA 0x4cd590: reset, optionally free the record, and return its address.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t record, std::uint8_t flags,
        IGameDataAiTextRecordRuntime& runtime);
};

}  // namespace pal4
