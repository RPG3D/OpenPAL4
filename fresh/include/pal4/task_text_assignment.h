#pragma once

#include <cstdint>

namespace pal4 {

class TaskTextAssignment final {
public:
    // IDA 0x593ea0 and 0x5944a0 share this reference-counted text body.
    template <typename Runtime>
    [[nodiscard]] static std::uint32_t Assign(
        const std::uint32_t object, const std::uint32_t source,
        const std::uint32_t control, const std::uint32_t context,
        const std::uint32_t context_offset, Runtime& runtime) {
        const auto length = runtime.CStringLength(source);
        if (length > 0xfffffffdu) runtime.ThrowLength();

        const auto data = runtime.ReadWord(object + 0x48u);
        bool shared = false;
        if (data != 0u) {
            const auto count = runtime.ReadByte(data - 1u);
            shared = count != 0u && count != 0xffu;
            if (shared && length == 0u) {
                runtime.WriteByte(data - 1u,
                                  static_cast<std::uint8_t>(count - 1u));
                runtime.WriteWord(object + 0x48u, 0u);
                runtime.WriteWord(object + 0x4cu, 0u);
                runtime.WriteWord(object + 0x50u, 0u);
                runtime.WriteWord(object + 0x54u, control);
                runtime.WriteWord(object + context_offset, context);
                return data;
            }
        }

        if (length == 0u) {
            runtime.SetErrorFlag(object + 0x44u, 1u);
            runtime.WriteWord(object + 0x54u, control);
            runtime.WriteWord(object + context_offset, context);
            return control;
        }

        if (shared) {
            runtime.ReallocateText(object + 0x44u, length);
        } else {
            const auto capacity = runtime.ReadWord(object + 0x50u);
            if (capacity > 0x1fu || capacity < length) {
                runtime.SetErrorFlag(object + 0x44u, 1u);
                runtime.ReallocateText(object + 0x44u, length);
            }
        }

        const auto destination = runtime.ReadWord(object + 0x48u);
        runtime.CopyBytes(destination, source, length);
        runtime.WriteWord(object + 0x4cu, length);
        runtime.WriteByte(destination + length, 0u);
        runtime.WriteWord(object + 0x54u, control);
        runtime.WriteWord(object + context_offset, context);
        return context;
    }
};

}  // namespace pal4
