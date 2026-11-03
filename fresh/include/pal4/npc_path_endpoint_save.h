#pragma once

#include "pal4/monster_path_endpoint_save.h"
#include "pal4/npc_path_endpoint_restore.h"

#include <cstdint>

namespace pal4 {

class NpcPathEndpointSave final {
public:
    // IDA 0x43a0d0: saves class-ID-zero NPC path state and releases the helper.
    [[nodiscard]] static std::uint32_t Release(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime);
};

class NpcPathEndpointC03Save final {
public:
    // IDA 0x43aad0: saves class-ID-one NPC path state.
    [[nodiscard]] static std::uint32_t Release(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime);
};

}  // namespace pal4
