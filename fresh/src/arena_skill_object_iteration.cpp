#include "pal4/arena_skill_manager.h"

namespace pal4 {

std::uint32_t ArenaSkillManager::ForEachObject(
    const std::uint32_t manager,
    const std::function<std::uint32_t(std::uint32_t, std::uint32_t)>& callback,
    const std::uint32_t argument, IArenaSkillObjectReader& runtime) {
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != runtime.ReadWord(manager + 8u); cursor += 4u) {
        if (callback(runtime.ReadWord(cursor), argument) == 0u) return 0u;
    }
    return runtime.ReadWord(manager + 8u);
}

}  // namespace pal4
