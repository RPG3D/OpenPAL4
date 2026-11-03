#include "pal4/effect_system_group_update.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

std::uint32_t IEffectSystemSecondaryTimerRuntime::NextPendingNode(
    std::uint32_t tree, std::uint32_t node) {
    return MonsterManagerTreeCursor::Successor(node, SharedNil(tree),
        [this](std::uint32_t address) { return ReadWord(address); }).next;
}

void EffectSystem::UpdateSecondaryTimers(
    std::uint32_t system, double seconds,
    IEffectSystemSecondaryTimerRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    const auto tree = system + 28u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        if (runtime.ReadFloat(node + 16u) > 0.0f) {
            const auto remaining = static_cast<float>(
                static_cast<double>(runtime.ReadFloat(node + 16u)) - seconds);
            runtime.WriteFloat(node + 16u, remaining);
            node = runtime.NextPendingNode(tree, node);
        } else {
            const auto effect = runtime.ReadWord(node + 12u);
            runtime.ResetDialog(effect);
            runtime.ReturnEffect(effect, 0u);
            node = runtime.EraseNode(tree, node);
        }
    }
}

void EffectSystem::UpdateObjectGroups(
    std::uint32_t system, std::uint32_t first,
    std::uint32_t second, IEffectSystemObjectGroupRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    constexpr std::uint32_t offsets[] = {
        68u, 92u, 80u, 116u, 128u, 152u,
        164u, 104u, 140u, 176u, 188u, 200u};
    for (const auto offset : offsets) {
        const auto head = runtime.ReadWord(system + offset + 4u);
        auto node = runtime.ReadWord(head);
        while (node != head) {
            runtime.UpdateObject(runtime.ReadWord(node + 8u),
                                 first, second);
            node = runtime.ReadWord(node);
        }
    }
}

}  // namespace pal4
