#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRecordTextRuntime {
public:
    virtual ~IGameDataRecordTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void ResetNested(std::uint32_t object) = 0;
};

class SkillDataRecordText final {
public:
    // IDA 0x4f6250: release four text fields in reverse record order.
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t record, IGameDataRecordTextRuntime& runtime);
};

class StuntDataRecordText final {
public:
    // IDA 0x4f6320: reset nested state, then release six text fields.
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t record, IGameDataRecordTextRuntime& runtime);
};

class MagicDataRecordText final {
public:
    // IDA 0x4f6470: reset nested state, then release six text fields.
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t record, IGameDataRecordTextRuntime& runtime);
};

}  // namespace pal4
