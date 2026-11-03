#pragma once

#include <cstdint>

namespace pal4 {

class ICombatSystemTextPropertyRuntime {
public:
    virtual ~ICombatSystemTextPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[noreturn]] virtual void ThrowRange() = 0;
    [[nodiscard]] virtual std::uint8_t DetachAndReset(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t record, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class CombatSystemTextProperties final {
public:
    static constexpr std::uint32_t kEmptyText = 0x841964u;

    // IDA 0x576810, 0x576950 and 0x576a90.
    [[nodiscard]] static std::uint8_t AssignFirst(
        std::uint32_t object, std::uint32_t source,
        ICombatSystemTextPropertyRuntime& runtime);
    [[nodiscard]] static std::uint8_t AssignSecond(
        std::uint32_t object, std::uint32_t source,
        ICombatSystemTextPropertyRuntime& runtime);
    [[nodiscard]] static std::uint8_t AssignThird(
        std::uint32_t object, std::uint32_t source,
        ICombatSystemTextPropertyRuntime& runtime);

private:
    [[nodiscard]] static std::uint8_t AssignAt(
        std::uint32_t destination, std::uint32_t source,
        ICombatSystemTextPropertyRuntime& runtime);
};

}  // namespace pal4
