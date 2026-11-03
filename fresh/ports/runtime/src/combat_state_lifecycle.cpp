#include "pal4/portable/combat_state_lifecycle.h"
#include <stdexcept>

namespace pal4::portable {
namespace {

class TransitionScopedServices final : public ICombatSceneUpdateServices {
public:
    TransitionScopedServices(ICombatSceneUpdateServices& services,
                             bool& transitioning)
        : services_(services), transitioning_(transitioning) {}

    void AdvanceCombat(float delta) override { services_.AdvanceCombat(delta); }
    void UpdateUI(double delta) override { services_.UpdateUI(delta); }
    std::uint32_t State() override { return services_.State(); }
    bool Alternate() override { return services_.Alternate(); }
    std::uint32_t Result() override { return services_.Result(); }
    void CleanupScriptCache() override { services_.CleanupScriptCache(); }
    std::uint32_t ConstructXmlDocument() override {
        return services_.ConstructXmlDocument();
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        return services_.DestroyXmlDocument(document);
    }
    void WriteResult(std::uint32_t document, std::uint32_t value,
                     bool tree_lookup) override {
        services_.WriteResult(document, value, tree_lookup);
    }
    void Transition(std::uint32_t state, std::uint32_t document) override {
        struct Reset final {
            bool& flag;
            ~Reset() { flag = false; }
        } reset{transitioning_};
        transitioning_ = true;
        services_.Transition(state, document);
    }

private:
    ICombatSceneUpdateServices& services_;
    bool& transitioning_;
};

} // namespace

CombatStateLifecycleBridge::CombatStateLifecycleBridge(
    CombatStateOwnerGeneration generation, ICombatStateReadiness& readiness,
    ICombatSceneServices& scene, ICombatSceneFrameRuntime& frame,
    ICombatSystemFrameRenderer& renderer, ICombatSceneUpdateServices& update)
    : generation_(generation), readiness_(readiness), scene_(scene),
      frame_(frame), renderer_(renderer), update_(update) {
    if (!generation_)
        throw std::invalid_argument("Combat bridge requires an owner generation");
    readiness_.ValidateRegisteredCombatOwner(generation_, kPalivCombatStateId);
}

CombatStateOwnerGeneration CombatStateLifecycleBridge::OwnerGeneration() const noexcept {
    return generation_;
}

std::uint32_t CombatStateLifecycleBridge::StateId() const noexcept {
    return kPalivCombatStateId;
}

CombatStateLifecycleBridge::Phase CombatStateLifecycleBridge::CurrentPhase() const noexcept {
    return phase_;
}

bool CombatStateLifecycleBridge::Active() const noexcept {
    return phase_ == Phase::Active;
}

bool CombatStateLifecycleBridge::NeedsLeave() const noexcept {
    return needs_leave_;
}

void CombatStateLifecycleBridge::ValidateCaller(
    CombatStateOwnerGeneration caller) const {
    if (caller != generation_)
        throw std::invalid_argument("Combat bridge owner generation mismatch");
}

void CombatStateLifecycleBridge::ValidateReady(
    CombatStateOwnerGeneration caller) const {
    ValidateCaller(caller);
    if (phase_ == Phase::Retired)
        throw std::logic_error("Combat bridge is retired");
    if (phase_ == Phase::Faulted)
        throw std::logic_error("Combat bridge failed and must be rebuilt");
    readiness_.ValidateRegisteredCombatOwner(generation_, kPalivCombatStateId);
}

void CombatStateLifecycleBridge::RequirePhase(
    Phase phase, const char* message) const {
    if (phase_ != phase) throw std::logic_error(message);
}

bool CombatStateLifecycleBridge::Enter(
    CombatStateOwnerGeneration caller, std::uint32_t previous_state,
    std::uint32_t request_document) {
    (void)previous_state; // The IDA 0x58ADB0 algorithm does not consume it.
    ValidateReady(caller);
    if (!request_document)
        throw std::invalid_argument("PALIV_COMBAT Entry requires request XML");
    RequirePhase(Phase::Ready, "PALIV_COMBAT Entry requires an inactive owner");
    if (entering_ || leaving_ || rendering_ || updating_)
        throw std::logic_error("PALIV_COMBAT Entry is not reentrant");
    entering_ = true;
    try {
        const auto accepted = CombatSceneCoordinator::EnterWithServices(
            request_document, scene_) != 0u;
        entering_ = false;
        phase_ = accepted ? Phase::Active : Phase::Faulted;
        needs_leave_ = accepted;
        return accepted;
    } catch (...) {
        entering_ = false;
        phase_ = Phase::Faulted;
        throw;
    }
}

void CombatStateLifecycleBridge::Leave(
    CombatStateOwnerGeneration caller, std::uint32_t next_state) {
    ValidateCaller(caller);
    if (phase_ == Phase::Retired)
        throw std::logic_error("Combat bridge is retired");
    readiness_.ValidateRegisteredCombatOwner(generation_, kPalivCombatStateId);
    if (!needs_leave_ || (phase_ != Phase::Active && phase_ != Phase::Faulted))
        throw std::logic_error("PALIV_COMBAT Leave requires an entered owner");
    if (entering_ || leaving_ || rendering_ ||
        (updating_ && !transitioning_))
        throw std::logic_error("PALIV_COMBAT Leave is not reentrant");
    leaving_ = true;
    try {
        (void)next_state;
        (void)CombatSceneCoordinator::LeaveWithServices(scene_);
        leaving_ = false;
        needs_leave_ = false;
        if (phase_ != Phase::Faulted) phase_ = Phase::Ready;
    } catch (...) {
        leaving_ = false;
        phase_ = Phase::Faulted;
        throw;
    }
}

void CombatStateLifecycleBridge::Render(
    CombatStateOwnerGeneration caller, std::uint32_t frame) {
    ValidateReady(caller);
    RequirePhase(Phase::Active, "PALIV_COMBAT Render requires an active owner");
    if (!frame) throw std::invalid_argument("PALIV_COMBAT Render requires a frame");
    if (entering_ || leaving_ || rendering_ || updating_)
        throw std::logic_error("PALIV_COMBAT Render is not reentrant");
    rendering_ = true;
    try {
        CombatSceneFrame::Render(frame, frame_, renderer_);
        rendering_ = false;
    } catch (...) {
        rendering_ = false;
        phase_ = Phase::Faulted;
        throw;
    }
}

std::uint32_t CombatStateLifecycleBridge::Update(
    CombatStateOwnerGeneration caller, float elapsed) {
    ValidateReady(caller);
    RequirePhase(Phase::Active, "PALIV_COMBAT Update requires an active owner");
    if (entering_ || leaving_ || rendering_ || updating_)
        throw std::logic_error("PALIV_COMBAT Update is not reentrant");
    updating_ = true;
    try {
        TransitionScopedServices transition_services(update_, transitioning_);
        const auto result = CombatSceneUpdate::Advance(elapsed, transition_services);
        updating_ = false;
        return result;
    } catch (...) {
        updating_ = false;
        phase_ = Phase::Faulted;
        throw;
    }
}

void CombatStateLifecycleBridge::Retire(CombatStateOwnerGeneration caller) {
    ValidateCaller(caller);
    if (phase_ == Phase::Retired) return;
    if (needs_leave_ || entering_ || leaving_ || rendering_ || updating_)
        throw std::logic_error("PALIV_COMBAT must be left and unbound before retirement");
    if (readiness_.IsRegisteredCombatOwner(generation_, kPalivCombatStateId))
        throw std::logic_error("PALIV_COMBAT dispatcher must be unbound before retirement");
    phase_ = Phase::Retired;
}

} // namespace pal4::portable
