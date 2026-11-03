#pragma once

#include "pal4/native_arena_collision_tree.h"
#include "pal4/player_manager_object.h"

#include <cstdint>

namespace pal4 {
class IBaseObjectMaterialRuntime;

// Runs the four-player enter callback against the current arena collision tree.
// Callback words in the actor image are original function identities; native
// movement dispatch must translate them before invoking the callbacks.
class NativeArenaPlayerSceneEntry final {
public:
    static constexpr std::uint32_t kEnterCallback = 0x51b760u;
    static void ForEach(PlayerManagerObjectState& players,
                        std::uint32_t arena,
                        NativeArenaCollisionTree& collision);
    static void UpdateData(PlayerManagerObjectState& players,
                           IBaseObjectMaterialRuntime* textures = nullptr);
    static void EnterAll(PlayerManagerObjectState& players,
                         std::uint32_t arena,
                         NativeArenaCollisionTree& collision,
                         IBaseObjectMaterialRuntime* textures = nullptr);
    static void ClearCollisionBindings(PlayerManagerObjectState& players);
};

}  // namespace pal4
