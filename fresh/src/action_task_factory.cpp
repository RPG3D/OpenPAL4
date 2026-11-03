#include "pal4/action_task_object.h"

namespace pal4 {

std::uint32_t ActionTaskFactory::ConstructActionDescriptor(
    const std::uint32_t descriptor, IActionTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, 0x848724u);
    return descriptor;
}

std::uint32_t ActionTaskFactory::ConstructHelperDescriptor(
    const std::uint32_t descriptor, IActionTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, 0x848720u);
    return descriptor;
}

std::uint32_t ActionTaskFactory::CreateAction(
    IActionTaskSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x5cu);
    if (memory == 0u) return 0u;
    try {
        return ActionTaskObject::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
