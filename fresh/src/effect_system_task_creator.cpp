#include "pal4/effect_system_task_creator.h"

namespace pal4 {

std::uint32_t EffectSystemTaskCreator::CreateFromDocument(
    std::uint32_t document, IEffectSystemTaskCreateRuntime& runtime) {
    if (document == 0u) return 0u;
    const auto class_name = runtime.StringAttribute(document, kClassAttribute);
    if (!class_name) return 0u;
    const auto factory = runtime.FindFactory(*class_name);
    if (factory == 0u) return 0u;
    const auto task = runtime.InvokeFactory(factory);
    if (task == 0u) return 0u;
    if (!runtime.InitializeTask(task, document)) {
        runtime.DeleteTask(task, 1u);
        return 0u;
    }
    return task;
}

}  // namespace pal4
