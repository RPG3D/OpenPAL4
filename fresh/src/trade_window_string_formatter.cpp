#include "pal4/trade_window_string_formatter.h"

#include <limits>
#include <stdexcept>

namespace pal4 {
namespace {

void ReleaseTemporary(const StringBufferState& temporary,
                      ITradeWindowStringFormatRuntime& runtime) {
    if (temporary.data == 0u) return;
    const auto header = temporary.data - 1u;
    const auto reference = runtime.ReadByte(header);
    if (reference == 0u || reference == 0xffu)
        runtime.Free(header);
    else
        runtime.WriteByte(header,
                          static_cast<std::uint8_t>(reference - 1u));
}

}  // namespace

StringBuffer& TradeWindowStringFormatter::Format(
    StringBuffer& destination, std::uint8_t& destination_flag,
    const std::uint32_t format, const std::uint32_t arguments,
    ITradeWindowStringFormatRuntime& runtime) {
    const auto formatted = runtime.Vsprintf(format, arguments, 1024u);
    if (formatted.size() >= 1024u)
        throw std::length_error("trade format exceeds stack buffer");

    StringBufferState temporary{};
    const auto length = static_cast<std::uint32_t>(formatted.size());
    const auto flag = runtime.StackFlagByte();
    if (runtime.AllocateTemporary(temporary, length, 1u)) {
        runtime.WriteTemporary(temporary.data, formatted);
        temporary.length = length;
        runtime.WriteByte(temporary.data + length, 0u);
    }

    destination_flag = flag;
    destination = StringBuffer{};
    try {
        const StringBuffer source(temporary);
        (void)destination.Assign(source, 0u,
                                 std::numeric_limits<std::uint32_t>::max(),
                                 runtime);
    } catch (...) {
        ReleaseTemporary(temporary, runtime);
        throw;
    }
    ReleaseTemporary(temporary, runtime);
    return destination;
}

}  // namespace pal4
