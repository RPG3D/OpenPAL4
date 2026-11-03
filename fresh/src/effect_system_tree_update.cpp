#include "pal4/effect_system_tree_update.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t Next(std::uint32_t tree, std::uint32_t node,
                   IEffectSystemTreeUpdateRuntime& runtime) {
    return MonsterManagerTreeCursor::Successor(
        node, runtime.SharedNil(tree),
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }).next;
}

}  // namespace

void EffectSystem::UpdateXmlEffects(
    std::uint32_t system, std::uint32_t first,
    std::uint32_t second, IEffectSystemTreeUpdateRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    const auto tree = system + 228u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        runtime.UpdateXmlObject(runtime.ReadWord(node + 16u),
                                first, second);
        node = Next(tree, node, runtime);
    }
}

void EffectSystem::UpdateInlineEffects(
    std::uint32_t system, std::uint32_t first,
    std::uint32_t second, IEffectSystemTreeUpdateRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    const auto tree = system + 212u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        if (runtime.ReadByte(node + 16u) != 0u)
            runtime.UpdateInlineObject(runtime.ReadWord(node + 12u),
                                       first, second);
        node = Next(tree, node, runtime);
    }
    (void)ProcessEffectList(system, runtime);
}

}  // namespace pal4
