#pragma once

#include <cstdint>

namespace pal4 {

class IGameQuestionRecordRuntime {
public:
    virtual ~IGameQuestionRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class GameQuestionRecord final {
public:
    // IDA 0x4fcd40: release four reference-counted text slots in reverse order.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t record, IGameQuestionRecordRuntime& runtime);
};

}  // namespace pal4
