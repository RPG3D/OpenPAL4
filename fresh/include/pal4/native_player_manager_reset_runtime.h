#pragma once

#include "pal4/pal_actor_control_loop.h"
#include "pal4/player_manager_operations.h"
#include "pal4/player_manager_slot.h"

namespace pal4 {

// Binds the four player-slot resets and actor clear to the rewritten objects.
// Other player-manager operations remain services of the supplied host.
class NativePlayerManagerResetRuntime final :
    public IPlayerManagerOperationsRuntime {
public:
    NativePlayerManagerResetRuntime(
        PlayerManagerObjectState& players, PalActorControlState& actor,
        IPlayerManagerSlotRuntime& slots,
        IPalActorControlLoopRuntime& actor_loop,
        IPlayerManagerOperationsRuntime& host)
        : players_(players), actor_(actor), slots_(slots),
          actor_loop_(actor_loop), host_(host) {}

    std::uint32_t UpdateInventory(std::uint32_t slot,
                                  std::uint32_t enabled) override;
    void ProcessGameData(std::uint32_t slot) override;
    void CallScriptIndex(std::uint32_t script, std::uint32_t virtual_slot,
                         std::uint32_t index) override;
    std::uint32_t SetStateFlag(std::uint32_t slot,
                               std::uint32_t value) override;
    void ApplyMaterialTexture(std::uint32_t slot) override;
    std::uint32_t ApplyMaterialTexture2(std::uint32_t slot) override;
    void CallSlotReset(std::uint32_t slot,
                       std::uint32_t virtual_slot) override;
    std::uint32_t ActorControl() override;
    std::uint32_t ClearActor(std::uint32_t control) override;
    std::uint32_t ForAllPlayers(std::uint32_t manager,
                                std::uint32_t callback,
                                std::uint32_t context) override;
    void CallSlotRender(std::uint32_t slot, std::uint32_t virtual_slot,
                        std::uint32_t argument) override;
    void RenderDebug(std::uint32_t actor_control) override;
    std::uint32_t CallSlotUpdate(std::uint32_t slot,
                                 std::uint32_t virtual_slot,
                                 std::uint32_t argument) override;
    std::uint32_t IterateRelated(std::uint32_t subobject,
                                 std::uint32_t argument) override;
    void CallSlotProcess(std::uint32_t slot,
                         std::uint32_t virtual_slot, float elapsed,
                         std::uint32_t argument) override;
    std::uint32_t UpdateControlFrame(std::uint32_t actor_control,
                                     float elapsed) override;

private:
    PlayerManagerObjectState& players_;
    PalActorControlState& actor_;
    IPlayerManagerSlotRuntime& slots_;
    IPalActorControlLoopRuntime& actor_loop_;
    IPlayerManagerOperationsRuntime& host_;
};

}  // namespace pal4
