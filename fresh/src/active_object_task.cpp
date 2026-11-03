#include "pal4/active_object_task.h"
#include "pal4/task_text_lifecycle.h"

namespace pal4 {

std::uint32_t ActiveObjectTask::Construct(
    const std::uint32_t object, IActiveObjectTaskRuntime& runtime) {
    return TaskTextLifecycle::Construct(object, 0x848728u, runtime);
}

std::uint32_t ActiveObjectTask::Destruct(
    const std::uint32_t object, IActiveObjectTaskRuntime& runtime) {
    return TaskTextLifecycle::Destruct(object, 0x848728u, runtime);
}

std::uint8_t ActiveObjectTask::Enter(
    const std::uint32_t object, IActiveObjectTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) return 0u;
    auto name = runtime.ReadWord(object + 0x48u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    const auto target = runtime.FindObjectByName(runtime.GameObjectManager(),
                                                  name);
    if (target != 0u) {
        const auto control = runtime.ReadWord(object + 0x54u);
        runtime.RemoveAnimatedObject(runtime.ArenaManager(), target);
        if (control != 0u) {
            runtime.InvokeObjectControl(target, control);
            runtime.AddAnimatedObject(runtime.ArenaManager(), target);
        }
    }
    runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t ActiveObjectTask::CheckStatus(
    const std::uint32_t object, const std::int32_t,
    IActiveObjectTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    return runtime.ReadByte(object + 0x3cu) == 0u ? 1u : 0u;
}

}  // namespace pal4
