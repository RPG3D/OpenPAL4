#pragma once

#include "pal4/monster_path_endpoint_save.h"
#include "pal4/monster_random_point_restore.h"

namespace pal4 {

class MonsterRandomPointSave final {
public:
    // IDA 0x4373e0: save and release the class-ID-three point target.
    [[nodiscard]] static std::uint32_t Release(
        const MonsterPathEndpointTargetState& target,
        const MonsterBehaviorManagerState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime,
        IMonsterRandomPointRuntime& point_runtime);
};

}  // namespace pal4
