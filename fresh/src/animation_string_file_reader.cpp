#include "pal4/animation_string_file_reader.h"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace pal4 {

bool AnimationStringFileReader::Load(
    StringBufferState& state, const std::uint32_t file,
    IAnimationStringFileRuntime& runtime) {
    std::uint32_t raw_length = 0;
    runtime.ReadLength(file, raw_length);
    if (raw_length >
        static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
        return false;

    std::array<char, 64> short_buffer{};
    std::vector<char> long_buffer;
    std::uint32_t temporary = 0;
    char* data = short_buffer.data();
    if (raw_length >= short_buffer.size()) {
        temporary = runtime.AllocateTemporary(raw_length + 1u);
        if (temporary == 0) return false;
        try {
            long_buffer.resize(static_cast<std::size_t>(raw_length) + 1u);
        } catch (...) {
            runtime.FreeTemporary(temporary);
            return false;
        }
        data = long_buffer.data();
    }

    const auto release_temporary = [&]() {
        if (temporary != 0) runtime.FreeTemporary(temporary);
    };
    if (!runtime.ReadPayload(file, data, raw_length)) {
        release_temporary();
        return false;
    }
    const auto length = static_cast<std::uint32_t>(
        std::find(data, data + raw_length + 1u, '\0') - data);

    const auto reference = state.data != 0
                               ? runtime.ReadReference(state.data - 1u) : 0u;
    const bool shared = state.data != 0 && reference != 0u &&
                        reference != 0xffu;
    if (length == 0) {
        if (shared) {
            runtime.WriteReference(state.data - 1u,
                                   static_cast<std::uint8_t>(reference - 1u));
            runtime.SetErrorFlag(state, 0u);
        } else {
            runtime.SetErrorFlag(state, 1u);
        }
        release_temporary();
        return true;
    }

    if (shared) {
        runtime.Reallocate(state, length);
    } else if (state.capacity > 31u || state.capacity < length ||
               state.data == 0) {
        runtime.SetErrorFlag(state, 1u);
        runtime.Reallocate(state, length);
    }
    if (state.data == 0) {
        release_temporary();
        return false;
    }
    runtime.WriteData(state.data, std::string_view(data, length));
    state.length = length;
    runtime.WriteByte(state.data + length, 0u);
    release_temporary();
    return true;
}

}  // namespace pal4
