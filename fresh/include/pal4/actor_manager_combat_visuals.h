#pragma once

#include <array>
#include <cstdint>

#include "pal4/actor_manager_combat_prepare.h"

namespace pal4 {

class IActorManagerCombatVisualRuntime {
public:
    virtual ~IActorManagerCombatVisualRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool CombatInactive(std::uint32_t combat) = 0;
    virtual void HandleExistingEffect(std::uint32_t handle,
                                      std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual ActorManagerEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) = 0;
    virtual void ResetTemporaryEffect(ActorManagerEffectHandle& handle) = 0;
    [[nodiscard]] virtual std::uint32_t CreateText(
        std::uint32_t literal) = 0;
    virtual void ReleaseText(std::uint32_t text) = 0;
    virtual void SetProperties(std::uint32_t handle,
                               std::uint32_t text,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ChildMatrix(
        std::uint32_t actor) = 0;
    virtual void UpdatePosition(std::uint32_t handle,
                                std::uint32_t position,
                                std::uint8_t mode) = 0;
    virtual void UpdateMatrix(std::uint32_t handle,
                              std::uint32_t matrix,
                              std::uint8_t mode) = 0;
    virtual void UpdateOrientation(
        std::uint32_t handle, const std::array<float, 3>& angles,
        std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t FinalizeEffect(
        std::uint32_t handle) = 0;
};

class ActorManagerCombatVisuals final {
public:
    static constexpr std::uint32_t kPosition = 0x84b1dcu;
    static constexpr std::uint32_t kT1Text = 0x8b5f50u;

    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t manager, std::uint32_t parameter,
        IActorManagerCombatVisualRuntime& runtime);
    static void ForwardEvent(std::uint32_t manager,
                             IActorManagerCombatVisualRuntime& runtime);
};

}  // namespace pal4
