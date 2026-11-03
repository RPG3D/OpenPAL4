#include "pal4/active_object_task.h"

#include "pal4/task_text_assignment.h"

namespace pal4 {

std::uint32_t ActiveObjectTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t name, const std::uint32_t control,
    const std::uint32_t context, IActiveObjectTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x40u, owner);
    runtime.WriteByte(object + 0x3cu, 0u);
    return TaskTextAssignment::Assign(object, name, control, context,
                                      0x58u, runtime);
}

}  // namespace pal4
