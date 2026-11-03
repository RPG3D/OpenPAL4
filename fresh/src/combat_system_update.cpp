#include "pal4/combat_system_update.h"

#include <bit>
#include <optional>

namespace pal4 {
bool CombatSystemUpdate::RequiresQuickResolution(std::uint8_t inactive,float seconds) noexcept {
    return inactive!=0u && seconds>=20.0f;
}
namespace {

class CandidateLease final {
public:
    CandidateLease(const std::uint32_t candidate,
                   ICombatSystemUpdateServices& runtime)
        : candidate_(candidate), runtime_(runtime) {}
    ~CandidateLease() {
        if (candidate_ != 0u) runtime_.ReleaseCandidate(candidate_, 1u);
    }

    CandidateLease(const CandidateLease&) = delete;
    CandidateLease& operator=(const CandidateLease&) = delete;

private:
    std::uint32_t candidate_;
    ICombatSystemUpdateServices& runtime_;
};

std::optional<std::uint32_t> AdvanceFrame(
    const std::uint32_t object, const float delta,
    const std::uint32_t quick_end_payload_seed,
    ICombatSystemUpdateServices& runtime) {
    const auto initial_state = runtime.ReadWord(object + 508u);
    if (initial_state >= 2u && initial_state <= 5u) {
        const auto previous = std::bit_cast<float>(
            runtime.ReadWord(object + 512u));
        runtime.WriteWord(object + 512u,
                          std::bit_cast<std::uint32_t>(previous + delta));
    }

    const auto quick_mode=runtime.ReadByte(object + 516u);
    if (quick_mode!=0u && CombatSystemUpdate::RequiresQuickResolution(quick_mode,
        std::bit_cast<float>(runtime.ReadWord(object + 512u)))) {
        const auto bucket = runtime.RandomNumber() % 1000u;
        if (bucket >= 500u) {
            const auto candidate = runtime.CreateQuickEndIterator(bucket);
            CandidateLease lease(candidate, runtime);
            if (runtime.GetCandidateCount(candidate) <= 0)
                static_cast<void>(runtime.DispatchScriptEvent(
                    "quick_end", quick_end_payload_seed));
            else
                runtime.RunCandidateAction(
                    runtime.GetCandidateActor(candidate));
        } else {
            static_cast<void>(runtime.DispatchScriptEvent(
                "quick_end", quick_end_payload_seed));
        }
    }

    const auto state = runtime.ReadWord(object + 508u);
    if (state >= 1u && state <= 5u) {
        runtime.UpdateTimedEffects(delta);
        runtime.UpdateActorData(delta);
        runtime.DispatchSystemUpdate(delta);
        static_cast<void>(runtime.UpdateConsoleAnimation(delta));
        const auto inactive = runtime.ReadByte(object + 516u);
        if (inactive != 0u) return inactive;
        runtime.UpdateWorldAndInput(delta);
        runtime.UpdateGameObject(delta);
        return std::nullopt;
    }
    if (state == 6u) {
        runtime.UpdateTimedEffects(delta);
        if (runtime.ReadByte(object + 516u) == 0u) {
            runtime.UpdateWorldAndInput(delta);
            static_cast<void>(runtime.UpdateGameObject(delta));
        }
        runtime.UpdateActorData(delta);
        runtime.UpdateConsoleAnimation(delta);
        return std::nullopt;
    }
    if (state == 7u) {
        runtime.CloseCombatUi(object);
        runtime.EndCombat(object);
        runtime.DispatchScriptEvent(
            "close_system", runtime.ReadWord(object + 504u));
        return std::nullopt;
    }
    return state - 1u;
}

// Preserves the original raw facade's acquisition count, argument bits and
// incidental returns while sharing all ordering/branches with native Advance.
class LegacyServices final : public ICombatSystemUpdateServices {
public:
    ICombatSystemUpdateRuntime& r;std::uint32_t object,last{};
    LegacyServices(std::uint32_t object,ICombatSystemUpdateRuntime& r):r(r),object(object){}
    std::uint32_t ReadWord(std::uint32_t a)override{return r.ReadWord(a);}
    void WriteWord(std::uint32_t a,std::uint32_t v)override{r.WriteWord(a,v);}
    std::uint8_t ReadByte(std::uint32_t a)override{return r.ReadByte(a);}
    std::uint32_t RandomNumber()override{return r.RandomNumber();}
    std::uint32_t CreateQuickEndIterator(std::uint32_t bucket)override{return r.SelectQuickEndCandidate(r.GetActorManager()+32u,bucket);}
    std::int32_t GetCandidateCount(std::uint32_t a)override{return r.GetCandidateCount(a);}
    std::uint32_t GetCandidateActor(std::uint32_t a)override{return r.GetCandidateActor(a);}
    void RunCandidateAction(std::uint32_t a)override{r.RunCandidateAction(a);}
    void ReleaseCandidate(std::uint32_t a,std::uint32_t f)override{r.ReleaseCandidate(a,f);}
    void DispatchScriptEvent(std::string_view name,std::uint32_t p)override{last=r.DispatchScriptEvent(name,p);}
    void UpdateTimedEffects(float dt)override{r.UpdateTimedEffects(dt);}
    void UpdateActorData(float dt)override{r.UpdateActorData(r.GetActorManager(),std::bit_cast<std::uint32_t>(dt));}
    void DispatchSystemUpdate(float dt)override{r.DispatchSystemUpdate(r.ReadWord(object+396u),std::bit_cast<std::uint32_t>(dt));}
    void UpdateConsoleAnimation(float dt)override{last=r.UpdateConsoleAnimation(dt);}
    void UpdateWorldAndInput(float dt)override{r.UpdateWorldAndInput(dt);}
    void UpdateGameObject(float dt)override{last=r.UpdateGameObject(dt);}
    void CloseCombatUi(std::uint32_t a)override{r.CloseCombatUi(a);}
    void EndCombat(std::uint32_t a)override{r.EndCombat(a);}
};
} // namespace
void CombatSystemUpdate::Advance(std::uint32_t object,float delta,
    std::uint32_t seed,ICombatSystemUpdateServices& services){
    (void)AdvanceFrame(object,delta,seed,services);
}
std::uint32_t CombatSystemUpdate::Update(std::uint32_t object,float delta,
    std::uint32_t seed,ICombatSystemUpdateRuntime& runtime){
    LegacyServices services(object,runtime);
    const auto immediate=AdvanceFrame(object,delta,seed,services);
    return immediate.value_or(services.last);
}

}  // namespace pal4
