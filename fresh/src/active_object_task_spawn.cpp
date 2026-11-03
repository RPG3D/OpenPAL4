#include "pal4/active_object_task.h"

#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"

namespace pal4 {

std::uint8_t ActiveObjectTask::SpawnFollowup(
    const std::uint32_t object,
    IActiveObjectTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x58u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x40u);
    if (owner == 0u) return 0u;
    const auto previous = runtime.ReadWord(object + 0x30u);
    if (previous != 0u) {
        runtime.NotifyPreviousTask(previous);
        return 1u;
    }

    const auto helper = ActionTaskFactory::CreateHelper(runtime);
    if (helper != 0u)
        runtime.AddHelperTask(context, helper, 1u, 0u, 0u);
    runtime.SetHelperOwner(helper, owner);
    TaskPointerCollection::Append(owner + 0x2a0u, helper, runtime);
    runtime.MarkProcessed(context);
    return 1u;
}

}  // namespace pal4
