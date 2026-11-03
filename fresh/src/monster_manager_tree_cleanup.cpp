#include "pal4/monster_manager_tree_cleanup.h"

namespace pal4 {

std::uint32_t MonsterManagerTreeCleanup::DestroySubtree(
    std::uint32_t root, IMonsterManagerTreeCleanupRuntime& runtime) {
    auto node = root;
    while (node != runtime.SharedNil()) {
        (void)DestroySubtree(runtime.ReadDword(node + 8u), runtime);
        const auto left = runtime.ReadDword(node);
        runtime.ReleaseTextRecord(node + 0x0cu);
        runtime.FreeMemory(node);
        node = left;
    }
    return runtime.SharedNil();
}

}  // namespace pal4
