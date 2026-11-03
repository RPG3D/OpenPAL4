#include "pal4/effect_system_active_frame.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t Next(std::uint32_t tree, std::uint32_t node,
                   IEffectSystemActiveFrameRuntime& runtime) {
    return runtime.NextActiveNode(tree, node);
}

class TemporaryHandle final {
public:
    TemporaryHandle(std::uint32_t id, std::uint32_t system,
                    IEffectSystemActiveFrameRuntime& runtime)
        : runtime_(runtime), value_(runtime.CopyHandle(id, system)) {}
    ~TemporaryHandle() { runtime_.ResetTemporaryHandle(value_); }
    [[nodiscard]] const EffectSystemHandle& Value() const { return value_; }

private:
    IEffectSystemActiveFrameRuntime& runtime_;
    EffectSystemHandle value_;
};

void SetId(std::uint32_t effect, std::uint32_t id,
           IEffectSystemActiveFrameRuntime& runtime) {
    const auto xml = effect + 20u;
    const auto attribute = runtime.FindXmlAttribute(xml, "ID");
    if (attribute != 0u)
        runtime.SetXmlInt(attribute, id);
    else
        runtime.AddXmlInt(xml, "ID", id);
}

std::uint32_t FinishAndErase(std::uint32_t system, std::uint32_t tree,
                             std::uint32_t node, std::uint32_t effect,
                             std::int32_t repeat_count,
                             IEffectSystemActiveFrameRuntime& runtime) {
    TemporaryHandle handle(runtime.ReadWord(node + 12u), system, runtime);
    runtime.NotifyFinished(system, handle.Value(), repeat_count);
    runtime.ResetDialog(effect);
    runtime.ReturnEffect(effect, 0u);
    runtime.WriteWord(node + 16u, 0u);
    return runtime.EraseActiveNode(tree, node);
}

}  // namespace

std::uint32_t IEffectSystemActiveFrameRuntime::NextActiveNode(
    std::uint32_t tree, std::uint32_t node) {
    return MonsterManagerTreeCursor::Successor(node, SharedNil(tree),
        [this](std::uint32_t address) { return ReadWord(address); }).next;
}

void EffectSystem::UpdateActiveEffects(
    std::uint32_t system, double seconds,
    IEffectSystemActiveFrameRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;

    const auto tree = system + 12u;
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto effect = runtime.ReadWord(node + 16u);
        if (runtime.ReadByte(effect + 80u) != 0u) {
            node = Next(tree, node, runtime);
            continue;
        }

        const auto repeats = static_cast<std::int32_t>(
            runtime.ReadWord(node + 20u));
        if (repeats == -1) {
            if (runtime.ReadWord(effect + 8u) == 0u) {
                (void)ProcessEffectList(system, runtime);
                runtime.WriteWord(effect + 12u, system);
                runtime.StartPlayback(effect);
                runtime.PausePlayback(effect);
                const auto id = runtime.ReadWord(node + 12u);
                SetId(effect, id, runtime);
                TemporaryHandle handle(id, system, runtime);
                runtime.NotifyStarted(system, handle.Value(), repeats);
            } else {
                runtime.UpdatePlayback(effect, seconds);
                if (runtime.ReadWord(effect + 8u) == 0u) {
                    (void)ProcessEffectList(system, runtime);
                    TemporaryHandle handle(runtime.ReadWord(node + 12u),
                                           system, runtime);
                    runtime.NotifyFinished(system, handle.Value(), repeats);
                }
            }
            node = Next(tree, node, runtime);
            continue;
        }

        if (repeats == 0) {
            node = FinishAndErase(system, tree, node, effect,
                                  repeats, runtime);
            continue;
        }
        if (repeats < 0) {
            node = Next(tree, node, runtime);
            continue;
        }

        if (runtime.ReadWord(effect + 8u) != 0u) {
            runtime.UpdatePlayback(effect, seconds);
            node = Next(tree, node, runtime);
            continue;
        }

        (void)ProcessEffectList(system, runtime);
        const auto remaining = repeats - 1;
        runtime.WriteWord(node + 20u,
                          static_cast<std::uint32_t>(remaining));
        if (remaining == 0) {
            node = FinishAndErase(system, tree, node, effect,
                                  remaining, runtime);
            continue;
        }

        runtime.WriteWord(effect + 12u, system);
        runtime.StartPlayback(effect);
        runtime.PausePlayback(effect);
        TemporaryHandle handle(runtime.ReadWord(node + 12u),
                               system, runtime);
        runtime.NotifyStarted(system, handle.Value(), remaining);
        node = Next(tree, node, runtime);
    }
}

}  // namespace pal4
