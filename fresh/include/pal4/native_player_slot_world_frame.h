#pragma once

#include <cstdint>

namespace pal4 {

class IAnimationEventQueueRuntime;
class IScriptPlayerPositionRuntime;
class IRwDictSchemaRuntime;
class NativeAnimatedActorAsset;
class NativeArenaCollisionTree;
class NativeBaseObjectMemory;
class NativeActorAnimationStateBinding;
class NativePlayerTransformRuntime;
struct PlayerManagerSlotState;

struct NativePlayerSlotWorldFrameResult {
    std::uint32_t resource_updates{};
    std::uint32_t animation_updates{};
    std::uint32_t collision_updates{};
    std::uint32_t queue_updates{};
};

// Binds one active player's +0x38 world-update slot to its owned resources.
class NativePlayerSlotWorldFrame final {
public:
    [[nodiscard]] static NativePlayerSlotWorldFrameResult Update(
        PlayerManagerSlotState& slot, float elapsed, std::uint32_t mode,
        const NativeAnimatedActorAsset& actor,
        NativeActorAnimationStateBinding& animation,
        NativeArenaCollisionTree& tree,
        NativePlayerTransformRuntime& transforms,
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        IAnimationEventQueueRuntime& queue,
        IScriptPlayerPositionRuntime* positions = nullptr);
};

}  // namespace pal4
