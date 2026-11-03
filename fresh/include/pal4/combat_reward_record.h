#pragma once

#include <cstdint>

namespace pal4 {

class ICombatRewardRecordRuntime {
public:
    virtual ~ICombatRewardRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
};

class CombatRewardRecord final {
public:
    // IDA 0x571560. The source pointer range must be valid and word-aligned.
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t destination, std::uint32_t source,
        ICombatRewardRecordRuntime& runtime);
};

}  // namespace pal4
