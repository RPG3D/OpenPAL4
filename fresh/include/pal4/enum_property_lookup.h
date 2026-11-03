#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct EnumPropertyState {
    std::uint32_t vtable{};
    std::uint32_t entries{};
    std::int32_t count{};
};
static_assert(sizeof(EnumPropertyState) == 12u);

struct EnumPropertyErrorState {
    std::uint32_t flag_word{};
    StringBufferState text{};
    std::uint32_t guard_word{};
};
static_assert(sizeof(EnumPropertyErrorState) == 20u);

class IEnumPropertyLookupRuntime {
public:
    virtual ~IEnumPropertyLookupRuntime() = default;
    [[nodiscard]] virtual std::int32_t MapIndex(
        const EnumPropertyState& property, std::int32_t input,
        std::uint32_t vtable_offset) = 0;
    virtual void RegisterAtExit(std::uint32_t callback_address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void SetErrorFlag(StringBufferState& text, std::uint8_t flag) = 0;
    virtual void Reallocate(StringBufferState& text, std::uint32_t length) = 0;
    virtual void Copy(std::uint32_t destination, std::string_view bytes) = 0;
    virtual void SetSize(StringBufferState& text, std::uint32_t length) = 0;
};

class EnumPropertyLookup final {
public:
    static constexpr std::uint32_t kErrorAddress = 0x90f958u;
    static constexpr std::uint32_t kCleanupAddress = 0x5ed770u;
    static constexpr std::string_view kErrorSuffix =
        " <ptnEnumProperty EnumError>";

    // IDA 0x5ed550 and 0x5ed770.
    [[nodiscard]] static std::uint32_t Lookup(
        const EnumPropertyState& property, std::int32_t input,
        EnumPropertyErrorState& error, IEnumPropertyLookupRuntime& runtime);
    static void Cleanup(EnumPropertyErrorState& error,
                        IEnumPropertyLookupRuntime& runtime);

private:
    static void AssignText(StringBufferState& text, std::string_view bytes,
                           IEnumPropertyLookupRuntime& runtime);
    static void AppendText(StringBufferState& text, std::string_view bytes,
                           IEnumPropertyLookupRuntime& runtime);
};

}  // namespace pal4
