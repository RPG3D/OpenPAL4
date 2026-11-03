#pragma once

#include <cstdint>

namespace pal4 {

class TaskTextLifecycle final {
public:
    // Shared object layout in IDA 0x593b30/0x593b80 and 0x593fc0/0x594010.
    template <typename Runtime>
    [[nodiscard]] static std::uint32_t Construct(
        const std::uint32_t object, const std::uint32_t vtable,
        Runtime& runtime) {
        runtime.BaseConstruct(object);
        runtime.WriteByte(object + 0x44u, runtime.TransientInitialByte());
        runtime.WriteWord(object + 0x48u, 0u);
        runtime.WriteWord(object + 0x4cu, 0u);
        runtime.WriteWord(object + 0x50u, 0u);
        runtime.WriteByte(object + 0x3cu, 0u);
        runtime.WriteWord(object, vtable);
        return object;
    }

    template <typename Runtime>
    [[nodiscard]] static std::uint32_t Destruct(
        const std::uint32_t object, const std::uint32_t vtable,
        Runtime& runtime) {
        runtime.WriteWord(object, vtable);
        runtime.WriteByte(object + 0x3cu, 0u);
        const auto data = runtime.ReadWord(object + 0x48u);
        if (data != 0u) {
            const auto count = runtime.ReadByte(data - 1u);
            if (count == 0u || count == 0xffu)
                runtime.FreeMemory(data - 1u);
            else
                runtime.WriteByte(data - 1u,
                                  static_cast<std::uint8_t>(count - 1u));
        }
        runtime.WriteWord(object + 0x48u, 0u);
        runtime.WriteWord(object + 0x4cu, 0u);
        runtime.WriteWord(object + 0x50u, 0u);
        runtime.WriteWord(object, 0x8486fcu);
        return runtime.BaseDestruct(object);
    }
};

}  // namespace pal4
