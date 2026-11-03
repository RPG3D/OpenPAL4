#include "pal4/gob_machine_trigger.h"

namespace pal4 {

std::uint32_t GobMachineTrigger::InitializeMovementAdapterAndCall(
    const std::uint32_t object, IGobMachineTriggerRuntime& runtime) {
    const auto clump = runtime.ReadWord(object + 0x14u);
    const auto adapter = runtime.MovementAdapterRoot();
    runtime.SetClumpRenderCallback(adapter, clump);
    return runtime.ForwardMovementSlot84(object);
}

std::uint32_t GobMachineTrigger::TryStartTaskRun(
    const std::uint32_t object, IGobMachineTriggerRuntime& runtime) {
    const auto running = runtime.ReadWord(object + 0x27cu);
    if (running != 0u) return running;
    const auto uses = runtime.ReadWord(object + 0x198u);
    if (uses == 0u) return uses;
    runtime.ClearTaskCollection(object + 0x258u);
    runtime.ClearTaskCollection(object + 0x268u);
    const auto conditions = runtime.CheckAndConsumeInventoryConditions(object);
    if (conditions == 0u) return conditions;
    const auto remaining = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x198u));
    if (remaining > 0)
        runtime.WriteWord(object + 0x198u,
                          static_cast<std::uint32_t>(remaining - 1));
    const auto started = runtime.StartTaskRun(object);
    if (started == 0u) return started;
    runtime.MarkEventProcessed(object + 0x258u);
    runtime.WriteWord(object + 0x278u, 0u);
    runtime.WriteWord(object + 0x27cu, 1u);
    runtime.WriteWord(object + 0x2b4u, 1u);
    if (runtime.ReadWord(object + 0x1b0u) != 4u) return 1u;

    const auto name = runtime.ArenaName() + "-" + runtime.SceneName() +
                      "-" + runtime.ObjectName(
                          runtime.ReadWord(object + 4u));
    runtime.DispatchGameEvent(runtime.Crc32(name));
    return runtime.ReleaseEventNameBuffer();
}

}  // namespace pal4
