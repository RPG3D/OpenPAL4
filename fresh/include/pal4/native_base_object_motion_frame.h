#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

struct BaseObjectState;
struct BaseObjectVector3;
class NativeArenaCollisionTree;
class NativePlayerTransformRuntime;
class NativeBaseObjectMemory;
class IRwDictSchemaRuntime;

struct NativeBaseObjectMotionFrameResult {
    std::uint32_t resource_updates{};
    std::uint32_t animation_updates{};
    std::uint32_t collision_updates{};
    std::uint32_t position_result{};
};

// Native dependencies of the shared 0x442970 position stage. The caller owns
// animation dispatch; behavior and animation event queues remain its concern.
class NativeBaseObjectMotionFrame final {
public:
    using AnimationUpdate = std::function<void(float)>;
    // Resolve the caller's vtable+60 override (players also refresh camera/audio).
    using PositionUpdate = std::function<std::uint32_t(const BaseObjectVector3&)>;
    [[nodiscard]] static NativeBaseObjectMotionFrameResult Update(
        BaseObjectState& base, float elapsed, std::uint32_t mode,
        NativeArenaCollisionTree& tree, NativePlayerTransformRuntime& transforms,
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        const AnimationUpdate& animation, const PositionUpdate& position = {});
};

}  // namespace pal4

