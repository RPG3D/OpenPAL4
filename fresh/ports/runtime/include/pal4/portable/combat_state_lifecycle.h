#pragma once

#include "pal4/combat_scene_coordinator.h"
#include "pal4/combat_scene_frame.h"
#include "pal4/combat_scene_update.h"
#include <cstdint>

namespace pal4::portable {

using CombatStateOwnerGeneration = std::uint64_t;

class ICombatStateReadiness {
public:
    virtual ~ICombatStateReadiness() = default;
    virtual void ValidateRegisteredCombatOwner(
        CombatStateOwnerGeneration generation,
        std::uint32_t state_id) const = 0;
    [[nodiscard]] virtual bool IsRegisteredCombatOwner(
        CombatStateOwnerGeneration generation,
        std::uint32_t state_id) const = 0;
};

// Borrows the same registered owner and platform services as the state router.
// All service references must outlive the bridge and be retired after unbinding.
class CombatStateLifecycleBridge final {
public:
    static constexpr std::uint32_t kPalivCombatStateId = 3u;

    enum class Phase { Ready, Active, Faulted, Retired };

    CombatStateLifecycleBridge(CombatStateOwnerGeneration,
        ICombatStateReadiness&, ICombatSceneServices&,
        ICombatSceneFrameRuntime&, ICombatSystemFrameRenderer&,
        ICombatSceneUpdateServices&);

    CombatStateLifecycleBridge(const CombatStateLifecycleBridge&) = delete;
    CombatStateLifecycleBridge& operator=(const CombatStateLifecycleBridge&) = delete;

    [[nodiscard]] CombatStateOwnerGeneration OwnerGeneration() const noexcept;
    [[nodiscard]] std::uint32_t StateId() const noexcept;
    [[nodiscard]] Phase CurrentPhase() const noexcept;
    [[nodiscard]] bool Active() const noexcept;
    [[nodiscard]] bool NeedsLeave() const noexcept;

    bool Enter(CombatStateOwnerGeneration caller,
        std::uint32_t previous_state, std::uint32_t request_document);
    void Leave(CombatStateOwnerGeneration caller, std::uint32_t next_state);
    void Render(CombatStateOwnerGeneration caller, std::uint32_t frame);
    // Returns CombatSceneUpdate's original XML destructor result, not a result state.
    std::uint32_t Update(CombatStateOwnerGeneration caller, float elapsed);

    // Call only after the host has unbound this dispatcher from PALGameIV.
    void Retire(CombatStateOwnerGeneration caller);

private:
    void ValidateCaller(CombatStateOwnerGeneration caller) const;
    void ValidateReady(CombatStateOwnerGeneration caller) const;
    void RequirePhase(Phase phase, const char* message) const;

    CombatStateOwnerGeneration generation_;
    ICombatStateReadiness& readiness_;
    ICombatSceneServices& scene_;
    ICombatSceneFrameRuntime& frame_;
    ICombatSystemFrameRenderer& renderer_;
    ICombatSceneUpdateServices& update_;
    Phase phase_{Phase::Ready};
    bool entering_{}, leaving_{}, rendering_{}, updating_{},
        transitioning_{}, needs_leave_{};
};

} // namespace pal4::portable
