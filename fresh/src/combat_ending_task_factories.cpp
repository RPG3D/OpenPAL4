#include "pal4/combat_ending_task_factories.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t CombatEndingTaskFactories::Bind(
    const CombatEndingTaskType type, const std::uint32_t factory,
    ICombatEndingTaskFactoryRuntime& runtime) {
    for (const auto& binding : kBindings) {
        if (binding.type == type) {
            runtime.WriteWord(factory, binding.vtable);
            return factory;
        }
    }
    throw std::invalid_argument("unknown combat ending task factory");
}

}  // namespace pal4
