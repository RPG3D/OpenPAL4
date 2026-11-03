#pragma once

#include "pal4/script_task_queue.h"

#include <cstdint>

namespace pal4 {

class ICombatActionRegistryRuntime : public IScriptTaskQueueRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ActorManagerSingleton() = 0;
};

class CombatActionRegistry final {
public:
    // IDA 0x5667b0: register an action in ActorManager's task queue.
    [[nodiscard]] static std::uint32_t RegisterActorEvent(
        std::uint32_t event, ICombatActionRegistryRuntime& runtime);
};

}  // namespace pal4
