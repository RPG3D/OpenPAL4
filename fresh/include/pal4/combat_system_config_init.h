#pragma once

#include <cstdint>

namespace pal4 {

class ICombatSystemConfigInitRuntime {
public:
    virtual ~ICombatSystemConfigInitRuntime() = default;
    virtual std::uint8_t AssignErrorText(std::uint32_t record,std::uint32_t length);
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t record, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        std::uint32_t record, std::uint8_t flag) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
};

class CombatSystemConfigInit final {
public:
    static constexpr std::uint32_t kErrorString = 0x8dc780u;

    // IDA 0x5796f0. The original stack byte is an explicit seed.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint8_t seed,
        ICombatSystemConfigInitRuntime& runtime);

    // IDA 0x579770.
    [[nodiscard]] static std::uint8_t InitializeStrings(
        std::uint32_t object, ICombatSystemConfigInitRuntime& runtime);

private:
    friend class ICombatSystemConfigInitRuntime;
    [[nodiscard]] static std::uint8_t AssignErrorString(
        std::uint32_t record, std::uint32_t length,
        ICombatSystemConfigInitRuntime& runtime);
};

}  // namespace pal4
