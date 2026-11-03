#include "pal4/combat_wounded_death_factory.h"

#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_wounded_sequence_task.h"

#include <string_view>

namespace pal4 {
namespace {

struct ConstructionGuard {
    IWoundedDeathFactoryRuntime& runtime;
    std::uint32_t object;
    bool container = false;
    bool name = false;

    ~ConstructionGuard() {
        if (object == 0u) return;
        if (name) runtime.CleanupName(object + 20u);
        if (container) runtime.DestroyContainer(object + 4u);
        runtime.Free(object);
    }
};

std::uint32_t CreateDeathTask(std::uint32_t task,
                              IWoundedDeathFactoryRuntime& runtime,
                              bool quick) {
    const auto gate_address = task + (quick ? 48u : 44u);
    if (!runtime.ActorCheckStateThree(runtime.ReadWord(gate_address)))
        return 0u;
    const auto object = runtime.Allocate(52u);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object};

    // Both constructors snapshot fields after allocation, in opposite order.
    const auto trailing_actor = runtime.ReadWord(
        task + (quick ? 44u : 48u));
    const auto leading_actor = runtime.ReadWord(
        task + (quick ? 48u : 44u));
    runtime.InitializeContainer(object + 4u);
    guard.container = true;
    // The original copies an unwritten stack byte at +20.
    runtime.WriteByte(object + 20u, 0u);
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    guard.name = true;
    runtime.SetNameErrorFlag(object + 20u, 1u);
    const std::string_view name = quick ? "QuickDead" : "Dead";
    runtime.ReallocateName(object + 20u,
                           static_cast<std::uint32_t>(name.size()));
    runtime.CopyName(object + 20u, name);
    runtime.SetNameSize(object + 20u,
                        static_cast<std::uint32_t>(name.size()));
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteWord(object + 48u, trailing_actor);
    runtime.WriteByte(object + 40u, 1u);
    runtime.WriteWord(object + 44u, leading_actor);
    runtime.WriteWord(object, quick ? QuickDeadSequenceTask::kVtable
                                    : DeadSequenceTask::kVtable);
    guard.object = 0u;
    return object;
}

}  // namespace

std::uint32_t WoundedSequenceTask::CreateDeathTask(
    std::uint32_t task, IWoundedDeathFactoryRuntime& runtime) {
    return pal4::CreateDeathTask(task, runtime, false);
}

std::uint32_t QuickWoundedSequenceTask::CreateDeathTask(
    std::uint32_t task, IWoundedDeathFactoryRuntime& runtime) {
    return pal4::CreateDeathTask(task, runtime, true);
}

}  // namespace pal4
