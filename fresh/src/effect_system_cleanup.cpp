#include "pal4/effect_system_cleanup.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t Next(std::uint32_t tree, std::uint32_t node,
                   IEffectSystemPruneRuntime& runtime) {
    return MonsterManagerTreeCursor::Successor(
        node, runtime.SharedNil(tree),
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }).next;
}

class TemporaryHandle final {
public:
    TemporaryHandle(std::uint32_t id, std::uint32_t system,
                    IEffectSystemCleanupEffectsRuntime& runtime)
        : runtime_(runtime), value_(runtime.CopyHandle(id, system)) {}
    ~TemporaryHandle() { runtime_.ResetTemporaryHandle(value_); }
    [[nodiscard]] const EffectSystemHandle& Value() const { return value_; }

private:
    IEffectSystemCleanupEffectsRuntime& runtime_;
    EffectSystemHandle value_;
};

void DestroyObjectTree(std::uint32_t tree,
                       std::uint32_t object_offset,
                       IEffectSystemCleanupEffectsRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto effect = runtime.ReadWord(node + object_offset);
        runtime.BeforeDestroyEffect(effect);
        runtime.DestroyEffect(effect, 1u);
        node = Next(tree, node, runtime);
    }
}

void ReleaseFirstActiveTree(std::uint32_t system,
                            IEffectSystemCleanupEffectsRuntime& runtime) {
    const auto tree = system + 12u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto effect = runtime.ReadWord(node + 16u);
        runtime.ResetDialog(effect);
        TemporaryHandle handle(runtime.ReadWord(node + 12u), system, runtime);
        runtime.NotifyRemoved(system, handle.Value(),
                              runtime.ReadWord(node + 20u));
        (void)EffectSystem::ProcessEffectList(system, runtime);
        runtime.ReturnEffect(effect, 0u);
        runtime.WriteWord(node + 16u, 0u);
        node = Next(tree, node, runtime);
    }
    runtime.ClearTree(tree, runtime.ReadWord(head), head);
}

void ReleaseSecondActiveTree(std::uint32_t system,
                             IEffectSystemCleanupEffectsRuntime& runtime) {
    const auto tree = system + 28u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto effect = runtime.ReadWord(node + 12u);
        runtime.ResetDialog(effect);
        (void)EffectSystem::ProcessEffectList(system, runtime);
        runtime.ReturnEffect(effect, 0u);
        node = Next(tree, node, runtime);
    }
    runtime.ClearTree(tree, runtime.ReadWord(head), head);
}

std::uint32_t ClearTimedTree(std::uint32_t tree,
                             IEffectSystemCleanupEffectsRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil(tree);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != nil) {
            runtime.DestroyTreeBranch(runtime.ReadWord(node + 8u));
            const auto left = runtime.ReadWord(node);
            runtime.FreeNode(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    } else {
        auto node = runtime.ReadWord(head);
        while (node != head) {
            const auto next = Next(tree, node, runtime);
            runtime.EraseNode(tree, node);
            node = next;
        }
    }
    return head;
}

}  // namespace

std::uint32_t EffectSystem::CleanupEffects(
    std::uint32_t system,
    IEffectSystemCleanupEffectsRuntime& runtime) {
    const auto xml_tree = system + 228u;
    DestroyObjectTree(xml_tree, 16u, runtime);
    const auto xml_head = runtime.ReadWord(xml_tree + 4u);
    runtime.ClearTree(xml_tree, runtime.ReadWord(xml_head), xml_head);

    ReleaseFirstActiveTree(system, runtime);
    ReleaseSecondActiveTree(system, runtime);

    const auto timed_tree = system + 244u;
    DestroyObjectTree(timed_tree, 12u, runtime);
    return ClearTimedTree(timed_tree, runtime);
}

std::uint32_t EffectSystem::ProcessEffectList(
    std::uint32_t system, IEffectSystemPruneRuntime& runtime) {
    const auto tree = system + 212u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        node = runtime.ReadByte(node + 16u) != 0u
            ? Next(tree, node, runtime)
            : runtime.RemoveInactiveInline(tree, node);
    }
    return node;
}

}  // namespace pal4
