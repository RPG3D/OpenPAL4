#pragma once

#include <array>
#include <cstdint>

#include "pal4/actor_manager_combat_prepare.h"

namespace pal4 {

class IActorManagerCombatPreviewRuntime {
public:
    virtual ~IActorManagerCombatPreviewRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool CombatInactive(std::uint32_t combat) = 0;
    [[nodiscard]] virtual bool CombatStateActive(std::uint32_t combat) = 0;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual ActorManagerEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool NeedsUpdate(
        const ActorManagerEffectHandle& effect) = 0;
    [[nodiscard]] virtual std::uint32_t CreateText(std::uint32_t literal) = 0;
    virtual void ReleaseText(std::uint32_t text) = 0;
    virtual void SetProperties(const ActorManagerEffectHandle& effect,
                               std::uint32_t text,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ChildMatrix(
        std::uint32_t actor) = 0;
    virtual void UpdatePosition(const ActorManagerEffectHandle& effect,
                                std::uint32_t position,
                                std::uint8_t mode) = 0;
    virtual void UpdateMatrix(const ActorManagerEffectHandle& effect,
                              std::uint32_t matrix,
                              std::uint8_t mode) = 0;
    virtual void UpdateOrientation(
        const ActorManagerEffectHandle& effect,
        const std::array<float, 3>& angles,
        std::uint8_t mode) = 0;
    virtual void FinalizeEffect(const ActorManagerEffectHandle& effect) = 0;
    virtual void ResetTemporaryEffect(ActorManagerEffectHandle& effect) = 0;
};

class ActorManagerCombatPreview final {
public:
    static constexpr std::uint32_t kCA46 = 0x8b67b0u;
    static constexpr std::uint32_t kEFF4 = 0x8b67b8u;
    static constexpr std::uint32_t kEFF3 = 0x8b67c0u;
    static constexpr std::uint32_t kEFF7 = 0x8b67c8u;
    static constexpr std::uint32_t kEFF6 = 0x8b67d0u;
    static constexpr std::uint32_t kT1Text = 0x8b5f50u;
    static constexpr std::uint32_t kPosition = 0x84b1dcu;

    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t manager, std::uint32_t event,
        IActorManagerCombatPreviewRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEff4(
        std::uint32_t manager, std::uint32_t event,
        IActorManagerCombatPreviewRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEff3(
        std::uint32_t manager, std::uint32_t event,
        IActorManagerCombatPreviewRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEff7(
        std::uint32_t manager, std::uint32_t parameter,
        IActorManagerCombatPreviewRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEff6(
        std::uint32_t manager, std::uint32_t parameter,
        IActorManagerCombatPreviewRuntime& runtime);
};

}  // namespace pal4
