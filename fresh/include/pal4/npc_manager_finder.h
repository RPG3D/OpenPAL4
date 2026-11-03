#pragma once

#include "pal4/monster_manager_finder.h"

namespace pal4 {

class NpcManagerFinder final {
public:
    // IDA 0x43cb70: the NPC tree has the same key/node layout and lookup flow.
    [[nodiscard]] static std::uint32_t FindByName(
        std::uint32_t manager, std::uint32_t name_address,
        IMonsterManagerFindRuntime& runtime);
};

}  // namespace pal4
