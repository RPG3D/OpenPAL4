#pragma once

#include <cstdint>

namespace pal4 {
struct GameWorldObjectState;
class NativeBaseObjectMemory;
class NativePalGameIVTextureStorage;
class NativeWorldAsset;

// Runs IDA661030 against the actual bound GameWorld and its native owners.
class NativeGameWorldCleanup final {
public:
    [[nodiscard]] static std::int32_t Cleanup(
        GameWorldObjectState& state, NativeWorldAsset& asset,
        NativeBaseObjectMemory& text_memory, NativePalGameIVTextureStorage& game);
};
}  // namespace pal4
