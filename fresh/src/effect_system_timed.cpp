#include "pal4/effect_system_timed.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t Next(std::uint32_t tree, std::uint32_t node,
                   IEffectSystemTimedRuntime& runtime) {
    return MonsterManagerTreeCursor::Successor(
        node, runtime.SharedNil(tree),
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }).next;
}

std::uint32_t Previous(std::uint32_t tree, std::uint32_t node,
                       IEffectSystemTimedRuntime& runtime) {
    return MonsterManagerTreeCursor::Predecessor(
        node, runtime.SharedNil(tree),
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }, 20u).next;
}

}  // namespace

bool EffectSystem::InsertTimedEffect(
    std::uint32_t system, std::uint32_t effect,
    float lifetime, IEffectSystemTimedRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return false;

    const auto tree = system + 244u;
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil(tree);
    auto parent = head;
    auto node = runtime.ReadWord(head + 4u);
    bool before = true;
    while (node != nil) {
        parent = node;
        before = effect < runtime.ReadWord(node + 12u);
        node = runtime.ReadWord(node + (before ? 0u : 8u));
    }

    if (runtime.ReadByte(tree + 8u) == 0u) {
        auto previous = parent;
        if (before && parent != runtime.ReadWord(head))
            previous = Previous(tree, parent, runtime);
        if (!(before && parent == runtime.ReadWord(head)) &&
            runtime.ReadWord(previous + 12u) >= effect)
            return false;
    }

    runtime.InsertTimedNode(tree, nil, parent, effect, lifetime);
    return true;
}

void EffectSystem::UpdateEffectTimers(
    std::uint32_t system, double seconds,
    IEffectSystemTimedRuntime& runtime) {
    const auto tree = system + 244u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto effect = runtime.ReadWord(node + 12u);
        runtime.UpdateTimedObject(effect, seconds);
        const auto remaining = static_cast<float>(
            static_cast<double>(runtime.ReadFloat(node + 16u)) - seconds);
        runtime.WriteFloat(node + 16u, remaining);
        if (remaining > 0.0f) {
            node = Next(tree, node, runtime);
        } else {
            runtime.BeforeDestroyEffect(effect);
            runtime.DestroyEffect(effect, 1u);
            node = runtime.EraseTimedNode(tree, node);
        }
    }
}

void EffectSystem::DrawTimedEffects(
    std::uint32_t system, std::uint32_t context,
    IEffectSystemTimedRuntime& runtime) {
    const auto tree = system + 244u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        runtime.DrawTimedObject(runtime.ReadWord(node + 12u), context);
        node = Next(tree, node, runtime);
    }
}

}  // namespace pal4
