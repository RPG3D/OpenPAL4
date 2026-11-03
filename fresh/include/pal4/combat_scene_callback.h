#pragma once

#include "pal4/combat_scene_coordinator.h"

#include <cstdint>

namespace pal4 {

class CombatSceneCallback final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8435d0u;
    static constexpr std::uint32_t kVtable = 0x8485e0u;

    // IDA 0x58b320.
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source, ICombatSceneRuntime& runtime);
};

}  // namespace pal4
