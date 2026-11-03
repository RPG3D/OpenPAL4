#include "pal4/npc_manager_finder.h"

namespace pal4 {

std::uint32_t NpcManagerFinder::FindByName(
    std::uint32_t manager, std::uint32_t name_address,
    IMonsterManagerFindRuntime& runtime) {
    return MonsterManagerFinder::FindByName(
        manager, name_address, runtime);
}

}  // namespace pal4
