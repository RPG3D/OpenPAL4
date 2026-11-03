#pragma once

#include <cstdint>

namespace pal4 {

class ICombatSystemTextRuntime {
public:
    virtual ~ICombatSystemTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class CombatSystemTextMembers final {
public:
    [[nodiscard]] static std::uint32_t ReleaseAt(
        std::uint32_t address, ICombatSystemTextRuntime& runtime);
    // IDA 0x5753c0.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t object, ICombatSystemTextRuntime& runtime);
};

}  // namespace pal4
