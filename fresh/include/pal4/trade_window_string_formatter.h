#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ITradeWindowStringFormatRuntime : public IStringAssignRuntime {
public:
    [[nodiscard]] virtual std::string Vsprintf(
        std::uint32_t format, std::uint32_t arguments,
        std::uint32_t buffer_bytes) = 0;
    [[nodiscard]] virtual bool AllocateTemporary(
        StringBufferState& state, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void WriteTemporary(std::uint32_t destination,
                                std::string_view text) = 0;
    [[nodiscard]] virtual std::uint8_t StackFlagByte() = 0;
};

class TradeWindowStringFormatter final {
public:
    // IDA 0x4969a0.
    [[nodiscard]] static StringBuffer& Format(
        StringBuffer& destination, std::uint8_t& destination_flag,
        std::uint32_t format, std::uint32_t arguments,
        ITradeWindowStringFormatRuntime& runtime);
};

}  // namespace pal4
