#pragma once

#include <cstdint>

#include "pal4/player_manager_object.h"

namespace pal4 {

class IPlayerManagerOperationsRuntime {
public:
    virtual ~IPlayerManagerOperationsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateInventory(
        std::uint32_t slot, std::uint32_t enabled) = 0;
    virtual void ProcessGameData(std::uint32_t slot) = 0;
    virtual void CallScriptIndex(std::uint32_t script,
        std::uint32_t virtual_slot, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SetStateFlag(
        std::uint32_t slot, std::uint32_t value) = 0;
    virtual void ApplyMaterialTexture(std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyMaterialTexture2(
        std::uint32_t slot) = 0;
    virtual void CallSlotReset(std::uint32_t slot,
        std::uint32_t virtual_slot) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::uint32_t ClearActor(
        std::uint32_t actor_control) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllPlayers(
        std::uint32_t manager, std::uint32_t callback,
        std::uint32_t context) = 0;
    virtual void CallSlotRender(std::uint32_t slot,
        std::uint32_t virtual_slot, std::uint32_t argument) = 0;
    virtual void RenderDebug(std::uint32_t actor_control) = 0;
    [[nodiscard]] virtual std::uint32_t CallSlotUpdate(
        std::uint32_t slot, std::uint32_t virtual_slot,
        std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t IterateRelated(
        std::uint32_t subobject, std::uint32_t argument) = 0;
    virtual void CallSlotProcess(std::uint32_t slot,
        std::uint32_t virtual_slot, float elapsed,
        std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateControlFrame(
        std::uint32_t actor_control, float elapsed) = 0;
};

class PlayerManagerOperationsRuntimeStub :
    public IPlayerManagerOperationsRuntime {
public:
    std::uint32_t UpdateInventory(std::uint32_t, std::uint32_t) override;
    void ProcessGameData(std::uint32_t) override;
    void CallScriptIndex(std::uint32_t, std::uint32_t,
                         std::uint32_t) override;
    std::uint32_t SetStateFlag(std::uint32_t, std::uint32_t) override;
    void ApplyMaterialTexture(std::uint32_t) override;
    std::uint32_t ApplyMaterialTexture2(std::uint32_t) override;
    void CallSlotReset(std::uint32_t, std::uint32_t) override;
    std::uint32_t ActorControl() override;
    std::uint32_t ClearActor(std::uint32_t) override;
    std::uint32_t ForAllPlayers(std::uint32_t, std::uint32_t,
                                std::uint32_t) override;
    void CallSlotRender(std::uint32_t, std::uint32_t,
                        std::uint32_t) override;
    void RenderDebug(std::uint32_t) override;
    std::uint32_t CallSlotUpdate(std::uint32_t, std::uint32_t,
                                 std::uint32_t) override;
    std::uint32_t IterateRelated(std::uint32_t,
                                 std::uint32_t) override;
    void CallSlotProcess(std::uint32_t, std::uint32_t,
                         float, std::uint32_t) override;
    std::uint32_t UpdateControlFrame(std::uint32_t, float) override;
};

class PlayerManagerOperations final {
public:
    static constexpr std::uint32_t kStateFlagOffset = 0x1f0u;
    static constexpr std::uint32_t kRelatedOffset = 0xaa8u;
    static constexpr std::uint32_t kScriptOffset = 0x654u;

    // IDA 0x441360.
    [[nodiscard]] static std::uint32_t ProcessLoadedData(
        std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x4413b0.
    [[nodiscard]] static std::uint32_t ResetData(
        std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x4413d0.
    [[nodiscard]] static std::uint32_t UpdateData(
        std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x441400.
    [[nodiscard]] static std::uint32_t ResetAllPlayerStates(
        std::uint32_t manager, IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x441430.
    [[nodiscard]] static std::uint32_t ForAllPlayers(
        std::uint32_t manager, std::uint32_t callback,
        std::uint32_t context, IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x441450.
    static void UpdateAndRender(const PlayerManagerObjectState& state,
        std::uint32_t manager, std::uint32_t argument,
        IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x441490.
    [[nodiscard]] static std::uint32_t ProcessPlayerUpdates(
        const PlayerManagerObjectState& state,
        std::uint32_t manager, std::uint32_t argument,
        IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x4414c0.
    [[nodiscard]] static std::uint32_t ProcessPlayerRelatedData(
        const PlayerManagerObjectState& state,
        std::uint32_t manager, std::uint32_t argument,
        IPlayerManagerOperationsRuntime& runtime);
    // IDA 0x441500.
    [[nodiscard]] static std::uint32_t ProcessGameElements(
        const PlayerManagerObjectState& state,
        std::uint32_t manager, float elapsed,
        std::uint32_t argument,
        IPlayerManagerOperationsRuntime& runtime);
};

}  // namespace pal4
