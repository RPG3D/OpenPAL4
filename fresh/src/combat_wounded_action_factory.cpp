#include "pal4/combat_wounded_action_factory.h"

#include "pal4/combat_wounded_sequence_task.h"

namespace pal4 {
namespace {

struct ConstructionGuard {
    IWoundedActionFactoryRuntime& runtime;
    std::uint32_t object;
    bool container_initialized = false;
    bool name_initialized = false;

    ~ConstructionGuard() {
        if (object == 0u) return;
        if (name_initialized) runtime.CleanupName(object + 20u);
        if (container_initialized) runtime.DestroyContainer(object + 4u);
        runtime.Free(object);
    }
};

}  // namespace

std::uint32_t CombatWoundedActionFactory::Create(
    const std::uint32_t source, const std::uint32_t target,
    IWoundedActionFactoryRuntime& runtime) {
    if (source == 0u || runtime.ActorInTerminalState(source)) return 0u;
    const auto quick = runtime.CombatInactive();

    bool final_hit = false;
    if (!quick && runtime.ActorActionKind(source) == 3u &&
        runtime.ActorWord(source, 804u) == 2u &&
        runtime.ActorWord(source, 1056u) != 0u) {
        final_hit = true;
    }

    const auto object = runtime.Allocate(WoundedSequenceTask::kSize);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object};
    runtime.InitializeContainer(object + 4u);
    guard.container_initialized = true;
    guard.name_initialized = true;

    if (quick) {
        runtime.InitializeModeName(
            object + 20u, "QuickWounded",
            static_cast<std::uint8_t>(source));
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, target);
        runtime.WriteWord(object + 48u, source);
        runtime.WriteWord(object, QuickWoundedSequenceTask::kVtable);
    } else {
        runtime.WriteByte(object + 20u, static_cast<std::uint8_t>(target));
        if (final_hit) {
            runtime.SetNameErrorFlag(object + 20u, 0u);
            runtime.SetExceptionMessage(object + 20u, "FinalHit");
        } else {
            runtime.WriteWord(object + 24u, 0u);
            runtime.WriteWord(object + 28u, 0u);
            runtime.WriteWord(object + 32u, 0u);
            runtime.SetExceptionMessage(object + 20u, "WoundedTask");
        }
        runtime.WriteWord(object + 36u, 0u);
        runtime.WriteByte(object + 40u, 1u);
        runtime.WriteWord(object + 44u, source);
        runtime.WriteWord(object + 48u, target);
        runtime.WriteWord(object, final_hit
            ? FinalHitSequenceTask::kVtable
            : WoundedSequenceTask::kVtable);
    }
    guard.object = 0u;
    return object;
}

}  // namespace pal4
