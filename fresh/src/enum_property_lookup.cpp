#include "pal4/enum_property_lookup.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace pal4 {

void EnumPropertyLookup::AssignText(StringBufferState& text,
                                    const std::string_view bytes,
                                    IEnumPropertyLookupRuntime& runtime) {
    const auto length = static_cast<std::uint32_t>(bytes.size());
    const auto reference = text.data == 0u ? 0u :
        runtime.ReadByte(text.data - 1u);
    const bool shared = reference != 0u && reference != 0xffu;
    if (shared && length == 0u) {
        runtime.WriteByte(text.data - 1u,
                          static_cast<std::uint8_t>(reference - 1u));
        runtime.SetErrorFlag(text, 0u);
        return;
    }
    if (!shared && length == 0u) {
        runtime.SetErrorFlag(text, 1u);
        return;
    }
    if (shared || text.capacity > 0x1fu || text.capacity < length) {
        if (!shared) runtime.SetErrorFlag(text, 1u);
        runtime.Reallocate(text, length);
    }
    runtime.Copy(text.data, bytes);
    runtime.SetSize(text, length);
}

void EnumPropertyLookup::AppendText(StringBufferState& text,
                                    const std::string_view bytes,
                                    IEnumPropertyLookupRuntime& runtime) {
    if (std::numeric_limits<std::uint32_t>::max() - text.length <=
        bytes.size()) {
        throw std::length_error("enum property error text");
    }
    if (bytes.empty()) return;
    const auto length = text.length +
        static_cast<std::uint32_t>(bytes.size());
    const auto reference = text.data == 0u ? 0u :
        runtime.ReadByte(text.data - 1u);
    const bool shared = reference != 0u && reference != 0xffu;
    if (shared || text.capacity < length) runtime.Reallocate(text, length);
    runtime.Copy(text.data + text.length, bytes);
    runtime.SetSize(text, length);
}

std::uint32_t EnumPropertyLookup::Lookup(
    const EnumPropertyState& property, const std::int32_t input,
    EnumPropertyErrorState& error, IEnumPropertyLookupRuntime& runtime) {
    const auto index = runtime.MapIndex(property, input, 0x10u);
    if (index >= 0 && index < property.count) {
        return property.entries + static_cast<std::uint32_t>(index) * 16u;
    }
    if ((error.guard_word & 1u) == 0u) {
        error.text = {};
        error.guard_word |= 1u;
        error.flag_word = (error.flag_word & ~0xffu) |
                          (static_cast<std::uint32_t>(input) & 0xffu);
        runtime.RegisterAtExit(kCleanupAddress);
    }
    AssignText(error.text, std::to_string(index), runtime);
    AppendText(error.text, kErrorSuffix, runtime);
    return kErrorAddress;
}

void EnumPropertyLookup::Cleanup(
    EnumPropertyErrorState& error, IEnumPropertyLookupRuntime& runtime) {
    if (error.text.data != 0u) {
        const auto header = error.text.data - 1u;
        const auto reference = runtime.ReadByte(header);
        if (reference == 0u || reference == 0xffu) {
            runtime.Free(header);
        } else {
            runtime.WriteByte(header,
                static_cast<std::uint8_t>(reference - 1u));
        }
    }
    error.text = {};
}

}  // namespace pal4
