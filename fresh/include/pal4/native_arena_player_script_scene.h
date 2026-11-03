#pragma once
#include <cstdint>
namespace pal4 {
struct PlayerManagerObjectState;
class NativeSceneMovementAsset;
class NativeArenaCollisionTree;
class IBaseObjectActivityRuntime;
// Script membership borrows the arena; LeaveAll retires its per-role bindings.
// IDA51C500/51C590. Shares the native release operation adapter.
class NativeArenaPlayerScriptScene final {
public:
    static std::uint32_t Add(PlayerManagerObjectState& players,
        std::uint32_t player, std::uint32_t arena,
        NativeSceneMovementAsset& movement, NativeArenaCollisionTree& collision,
        IBaseObjectActivityRuntime& activity);
    static std::uint32_t Remove(PlayerManagerObjectState& players,
        std::uint32_t player, std::uint32_t arena,
        NativeSceneMovementAsset& movement, IBaseObjectActivityRuntime& activity);
};
}  // namespace pal4
