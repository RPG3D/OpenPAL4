#include "pal4/string_uppercase.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t StringUppercase::Convert(
    UppercaseStringState& state, IStringUppercaseRuntime& runtime) {
    const auto temporary = runtime.Duplicate(
        state.data != 0u ? state.data : runtime.EmptySource());
    const auto uppercase = runtime.ToUpper(temporary);
    const auto length = runtime.Length(uppercase);
    if (length > 0xfffffffdu) throw std::length_error("string too long");

    if (state.data != 0u) {
        const auto references = runtime.ReadByte(state.data - 1u);
        if (references != 0u && references != 0xffu) {
            if (length == 0u) {
                runtime.WriteByte(state.data - 1u,
                                  static_cast<std::uint8_t>(references - 1u));
                state.data = 0u;
                state.length = 0u;
                state.capacity = 0u;
                return runtime.Free(temporary);
            }
            runtime.Reallocate(state, length);
            runtime.Copy(state.data, uppercase, length);
            state.length = length;
            runtime.WriteByte(state.data + length, 0u);
            return runtime.Free(temporary);
        }
    }

    if (length == 0u) {
        runtime.SetErrorFlag(state, 1u);
        return runtime.Free(temporary);
    }
    if (state.capacity > 31u || state.capacity < length) {
        runtime.SetErrorFlag(state, 1u);
        runtime.Reallocate(state, length);
    }
    runtime.Copy(state.data, uppercase, length);
    state.length = length;
    runtime.WriteByte(state.data + length, 0u);
    return runtime.Free(temporary);
}

}  // namespace pal4
