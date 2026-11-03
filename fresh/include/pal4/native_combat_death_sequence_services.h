#pragma once
#include "pal4/combat_script_task_operations.h"
#include "pal4/combat_status_tasks.h"
#include <stdexcept>
namespace pal4 {
// Borrowed actual actor/event owners. Receivers validate actor identity and
// dispatch original event virtual8 SET and actor544 death lifecycle.
class INativeCombatDeathSequenceServices : public ICombatScriptTaskRuntime,
                                          public ICombatStatusTaskRuntime {
public:
    virtual void SetActorState(std::uint32_t actor,std::uint32_t state,std::uint32_t notify)=0;
    [[nodiscard]] virtual std::uint32_t EventDataByIndex(std::uint32_t index)=0;
    virtual void ApplyFinalHitPropertyAction(std::uint32_t) {
        throw std::logic_error("FinalHit actor property action is not bound");
    }
};
}
