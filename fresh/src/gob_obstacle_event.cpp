#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_event.h"
#include "pal4/gob_obstacle_snapshot.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t GobObstacle::RunScriptAndProcessEvent(
    const std::uint32_t object, const std::uint32_t event,
    IGobObstacleEventRuntime& runtime) {
    const auto uses = runtime.ReadWord(object + 0x198u);
    if (uses == 0u) return 0u;
    const auto length = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x17cu));
    if (length > 0) {
        runtime.InitializeScript();
        const auto path = runtime.CurrentSceneScriptPath();
        const auto entry = runtime.ReadCString(
            runtime.ReadWord(object + 0x178u)).substr(0, length);
        const auto key = path + "[" + std::string(entry) + "]";
        if (!runtime.ScriptCacheContains(key)) {
            const auto interpreter = runtime.CreateScriptInterpreter(
                path, key, entry);
            if (interpreter != 0u &&
                runtime.HandleScriptExecution(interpreter)) {
                runtime.ReportScriptException(interpreter);
                runtime.ReleaseScriptException(interpreter);
            } else {
                runtime.ShowScriptNotFound();
            }
        }
        runtime.CleanupScriptPackages();
        runtime.CleanupScriptPath();
    }
    runtime.TryTriggerTaskRun(object, event);
    const auto pending = runtime.ReadWord(object + 0x260u);
    if (static_cast<std::int32_t>(pending) <= 0)
        return runtime.ConsumeResearchUse(object);
    return pending;
}

std::uint32_t GobObstacle::ResetTriggeredRuntime(
    const std::uint32_t object, IGobObstacleEventRuntime& runtime) {
    return runtime.ResetMachineTriggeredRuntime(object);
}

std::uint32_t GobObstacle::CreateStateSnapshot(
    const std::uint32_t object, IGobObstacleEventRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    auto& snapshot_runtime = runtime.SnapshotRuntime();
    const auto snapshot = GobObstacleStateSnapshot::Create(snapshot_runtime);
    if (snapshot == 0u) return 0u;
    snapshot_runtime.AssignNameRecord(snapshot + 4u,
        runtime.ReadCString(runtime.ReadWord(object + 4u)));
    return snapshot;
}

}  // namespace pal4
