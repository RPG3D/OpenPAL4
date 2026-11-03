#include "pal4/combat_state_reset.h"

namespace pal4 {

void CombatStateReset::Advance(ICombatStateResetServices& runtime) {
    if (!runtime.Active()) return;
    runtime.TouchSkillSystem();
    runtime.ResetTaskFlags();
    runtime.CleanupTaskQueue();
    const auto iterator = runtime.FirstMonsterIterator();
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorCurrent(iterator);
            const auto value = runtime.ActorProperty(actor, 7u);
            runtime.ApplyPropertyDelta(actor, 7u, 0u - value);
            runtime.ResetActor(actor, actor);
            runtime.IteratorAdvance(iterator);
        }
        runtime.DispatchCombatOverOne();
    } catch (...) {
        if (iterator != 0u) runtime.DestroyIterator(iterator);
        throw;
    }
    if (iterator != 0u) runtime.DestroyIterator(iterator);
}
namespace {
class LegacyServices final : public ICombatStateResetServices {
public:
    ICombatStateResetRuntime& r;std::uint32_t result{};
    explicit LegacyServices(ICombatStateResetRuntime& r):r(r){}
    bool Active()override{return r.IsCombatStateActive(r.CombatSystem())!=0u;}
    void TouchSkillSystem()override{r.NullSub91(r.SkillSystemInstance(1u,"cbStageDirectCenter::onQuickEnd","QuickEndCombat!!"));}
    void ResetTaskFlags()override{r.ResetTaskFlags(r.ActorManager()+96u);}
    void CleanupTaskQueue()override{r.CleanupTaskQueue(r.ActorManager()+96u);}
    std::uint32_t FirstMonsterIterator()override{return r.FirstMonsterIterator(r.ActorManager());}
    bool IteratorAtEnd(std::uint32_t t)override{return r.IteratorAtEnd(t);}
    std::uint32_t IteratorCurrent(std::uint32_t t)override{return r.IteratorCurrent(t);}
    void IteratorAdvance(std::uint32_t t)override{r.IteratorAdvance(t);}
    void DestroyIterator(std::uint32_t t)override{result=r.DestroyIterator(t);}
    std::uint32_t ActorProperty(std::uint32_t a,std::uint32_t i)override{return r.ActorProperty(a,i);}
    void ApplyPropertyDelta(std::uint32_t a,std::uint32_t i,std::uint32_t bits)override{r.ApplyPropertyDelta(a,i,bits);}
    void ResetActor(std::uint32_t a,std::uint32_t arg)override{r.ResetActor(a,arg);}
    void DispatchCombatOverOne()override{result=r.ProcessScriptEvent(r.ScriptEventRouter(),0x8b8b8cu,1u,1u);}
};
}
std::uint8_t CombatStateReset::Reset(std::uint32_t,ICombatStateResetRuntime& runtime){
    LegacyServices services(runtime);Advance(services);return static_cast<std::uint8_t>(services.result);
}

void CombatStateReset::NullSub3(
    [[maybe_unused]] std::uint32_t argument) {}

}  // namespace pal4
