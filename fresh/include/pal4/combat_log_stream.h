#pragma once

#include <cstdint>
#include <iosfwd>

namespace pal4 {

class ICombatLogTextRuntime;

class ICombatLogIosRuntime {
public:
    virtual ~ICombatLogIosRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void InitializeBase(std::uint32_t ios) = 0;
    virtual void Clear(std::uint32_t ios, std::uint32_t state,
                       bool raise) = 0;
    virtual void AddStandardStream(std::uint32_t ios) = 0;
};

class CombatLogStream final {
public:
    // IDA 0x57e7e0/0x57e950: MSVC ostream put and flush behavior.
    [[nodiscard]] static std::ostream& WriteEntry(std::ostream& stream,
                                                    std::uint8_t character);
    [[nodiscard]] static std::ostream& Flush(std::ostream& stream);
    // IDA 0x57f7a0: formatted single-byte insertion with width/fill handling.
    [[nodiscard]] static std::ostream& WriteFormatted(
        std::ostream& stream, std::uint8_t character);
    // IDA 0x57e9b0.
    [[nodiscard]] static std::uint32_t GetString(
        std::uint32_t stream, std::uint32_t result,
        ICombatLogTextRuntime& runtime);
    // IDA 0x57f2d0.
    [[nodiscard]] static std::uint32_t InitializeIos(
        std::uint32_t ios, std::uint32_t buffer,
        std::uint8_t standard_stream, ICombatLogIosRuntime& runtime);
};

}  // namespace pal4
