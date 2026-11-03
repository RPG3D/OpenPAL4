#pragma once

#include <cstdint>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeGameSaveArchiveReader;
class INativeArenaSceneLoadServices;

class NativeArenaConfigLoadRuntime final {
public:
    // IDA51B000: borrows a live arena image, continuous input and required scene
    // services. Failure/exception can leave incremental arena/scene changes;
    // this is not an atomic restore or a complete GameSaveReader composition.
    [[nodiscard]] static bool Deserialize(
        std::uint32_t arena, NativeBaseObjectMemory& memory,
        NativeGameSaveArchiveReader& archive,
        INativeArenaSceneLoadServices& services);
};
}  // namespace pal4
