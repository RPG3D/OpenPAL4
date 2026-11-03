#pragma once

#include "pal4/game_object_lifecycle.h"
#include "pal4/monster_path_endpoint_restore.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct NpcPathEndpointTargetState {
    std::array<std::uint32_t, 10> words{};
};
static_assert(sizeof(NpcPathEndpointTargetState) == 0x28u);

class NpcPathEndpointRestoreRuntimeStub
    : public MonsterPathEndpointRestoreRuntimeStub {};

class NpcPathEndpointRestore final {
public:
    // IDA 0x4399f0: returns the final EAX register value.
    [[nodiscard]] static std::uint32_t Accept(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointRestoreRuntime& runtime);
};

class NpcPathEndpointC03Restore final {
public:
    // IDA 0x43a3e0: restores class-ID-one NPC path state.
    [[nodiscard]] static std::uint32_t Accept(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointRestoreRuntime& runtime);
};

}  // namespace pal4
