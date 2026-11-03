#pragma once

#include <cstdint>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeGameSaveArchiveReader;

// Persistent collection buffers belong to the player's memory owner. The
// archive cursor must point at MGC_HOST_SER_FLAG on entry.
class NativeHostServerLoadRuntime final {
public:
    [[nodiscard]] static bool Deserialize(
        std::uint32_t player_slot, NativeBaseObjectMemory& memory,
        NativeGameSaveArchiveReader& archive);
};
}  // namespace pal4
