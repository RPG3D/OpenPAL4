#include "pal4/game_arena_string_buffer.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t GameArenaStringBuffer::Copy(
    std::uint32_t destination, std::uint32_t arena_offset,
    IGameArenaStringBufferRuntime& runtime) {
    const auto source = runtime.ReadWord(runtime.ArenaManager() + arena_offset);
    runtime.InitializeBuffer(destination);
    const auto length = runtime.StringLength(source);
    if (length > 0xfffffffdu) throw std::length_error("arena string too long");

    bool prefix_reallocate = false;
    const auto data = runtime.ReadWord(destination + 4u);
    if (data != 0u) {
        const auto prefix = runtime.ReadByte(data - 1u);
        if (prefix != 0u && prefix != 0xffu) {
            if (length == 0u) {
                runtime.WriteByte(data - 1u,
                                  static_cast<std::uint8_t>(prefix - 1u));
                runtime.SetErrorFlag(destination, 0u);
                return destination;
            }
            prefix_reallocate = true;
        }
    }

    if (length == 0u) {
        runtime.SetErrorFlag(destination, 1u);
        return destination;
    }
    if (prefix_reallocate) {
        runtime.Reallocate(destination, length);
    } else {
        const auto capacity = runtime.ReadWord(destination + 0xcu);
        if (capacity > 0x1fu || capacity < length) {
            runtime.SetErrorFlag(destination, 1u);
            runtime.Reallocate(destination, length);
        }
    }
    runtime.CopyBytes(runtime.ReadWord(destination + 4u), source, length);
    runtime.WriteWord(destination + 8u, length);
    runtime.WriteByte(runtime.ReadWord(destination + 4u) + length, 0u);
    return destination;
}

std::uint32_t GameArenaStringBuffer::First(
    std::uint32_t destination, IGameArenaStringBufferRuntime& runtime) {
    return Copy(destination, 8u, runtime);
}

std::uint32_t GameArenaStringBuffer::Second(
    std::uint32_t destination, IGameArenaStringBufferRuntime& runtime) {
    return Copy(destination, 0xe0u, runtime);
}

}  // namespace pal4
