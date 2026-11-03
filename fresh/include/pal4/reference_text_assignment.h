#pragma once

#include <cstdint>

namespace pal4::detail {

enum class TextAssignmentResult { shared_empty, empty, nonempty };

struct TextAssignment {
    TextAssignmentResult branch;
    std::uint32_t native_result;
};

template <typename Runtime>
TextAssignment AssignText(const std::uint32_t record,
                          const std::uint32_t source,
                          Runtime& runtime) {
    const auto length = runtime.CStringLength(source);
    if (length > 0xfffffffdu) runtime.ThrowLength();
    const auto data = runtime.ReadWord(record + 4u);
    bool shared = false;
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        shared = count != 0u && count != 0xffu;
        if (shared && length == 0u) {
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
            return {TextAssignmentResult::shared_empty,
                    runtime.SetErrorFlag(record, 0u)};
        }
    }
    if (length == 0u)
        return {TextAssignmentResult::empty,
                runtime.SetErrorFlag(record, 1u)};
    if (shared) {
        runtime.ReallocateText(record, length);
    } else {
        const auto capacity = runtime.ReadWord(record + 12u);
        if (capacity > 0x1fu || capacity < length) {
            (void)runtime.SetErrorFlag(record, 1u);
            runtime.ReallocateText(record, length);
        }
    }
    const auto destination = runtime.ReadWord(record + 4u);
    runtime.CopyBytes(destination, source, length);
    runtime.WriteWord(record + 8u, length);
    runtime.WriteByte(destination + length, 0u);
    return {TextAssignmentResult::nonempty, 0u};
}

}  // namespace pal4::detail
