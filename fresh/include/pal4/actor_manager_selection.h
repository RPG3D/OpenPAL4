#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorManagerHandle {
    std::uint32_t first = 0;
    std::uint32_t second = 0;
};

class IActorManagerActionProgressRuntime {
public:
    virtual ~IActorManagerActionProgressRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual double CriticalDamageMultiplier(std::uint32_t actor) = 0;
    virtual void TouchSkillSystem() = 0;
};

class IActorManagerSelectionVisualRuntime {
public:
    virtual ~IActorManagerSelectionVisualRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void AdjustScale(std::uint32_t actor,
                             std::array<float, 3>& scale) = 0;
    virtual void UpdateDialog(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual ActorManagerHandle BuildDialog(
        std::uint32_t actor, std::string_view text,
        std::uint32_t context) = 0;
    virtual void ResetTemporaryHandle() = 0;
    virtual void UpdateEffectProperties(
        std::uint32_t dialog, const std::array<float, 3>& scale,
        std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::array<float, 8> ActorBoundingBox(
        std::uint32_t actor) = 0;
    virtual void HandleEffect(std::uint32_t handle,
                              std::uint32_t actor, bool force) = 0;
    [[nodiscard]] virtual std::uint32_t CombatCamera() = 0;
    [[nodiscard]] virtual ActorManagerHandle CreateEffect(
        std::uint32_t camera, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t first_flag, std::uint32_t second_flag) = 0;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::string_view event, std::uint32_t actor,
        std::uint32_t mode) = 0;
};

// Existing full clients retain their action-progress interface.
class IActorManagerSelectionRuntime : public IActorManagerActionProgressRuntime,
                                     public IActorManagerSelectionVisualRuntime {};

class ActorManagerSelection final {
public:
    [[nodiscard]] static std::uint32_t SetSelected(
        std::uint32_t actor, std::uint32_t selected,
        IActorManagerSelectionVisualRuntime& runtime);
    [[nodiscard]] static double CriticalRatio(
        std::uint32_t facet, IActorManagerActionProgressRuntime& runtime);
    [[nodiscard]] static bool IsStateOne(
        std::uint32_t facet, IActorManagerActionProgressRuntime& runtime);
};

}  // namespace pal4
