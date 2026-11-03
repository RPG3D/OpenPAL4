#pragma once

#include "pal4/native_player_slot_world_frame.h"

#include <cstdint>

namespace pal4 {

class IAnimationEventQueueRuntime;
class IScriptPlayerPositionRuntime;
class IRwDictSchemaRuntime;
class NativeActorControlSession;
class NativeArenaCollisionTree;
class NativeBaseObjectMemory;
class NativeGameArenaManagerOwner;
class NativePlayerActorAssets;
class NativePlayerAnimationSet;
class NativeActorAnimationStateBinding;
class NativePlayerManagerSession;
class NativePlayerTransformRuntime;
class IPalActorControlFrameRuntime;

struct NativePlayerManagerWorldFrameResult {
    std::uint32_t slot_updates{};
    std::uint32_t control_updates{};
    NativePlayerSlotWorldFrameResult slot{};
};

// Binds IDA 0x441500 to the owned player slots and ActorControl state.
class NativePlayerManagerWorldFrame final {
public:
    [[nodiscard]] static NativePlayerManagerWorldFrameResult Update(
        NativePlayerManagerSession& players, NativePlayerActorAssets& assets,
        NativeActorAnimationStateBinding& animation,
        NativeArenaCollisionTree& tree,
        NativePlayerTransformRuntime& transforms,
        NativeActorControlSession& control,
        NativeGameArenaManagerOwner& arena,
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        IAnimationEventQueueRuntime& queue, float elapsed,
        std::uint32_t mode,
        IPalActorControlFrameRuntime* control_frame = nullptr,
        NativePlayerAnimationSet* animations = nullptr,
        IScriptPlayerPositionRuntime* positions = nullptr);
};

}  // namespace pal4
