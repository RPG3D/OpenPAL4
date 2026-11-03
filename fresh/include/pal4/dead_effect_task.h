#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct DeadEffectHandle {
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
};

class IDeadEffectTaskRuntime {
public:
    virtual ~IDeadEffectTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t CombatCamera(
        std::uint32_t combat) = 0;
    [[nodiscard]] virtual DeadEffectHandle CreateEffect(
        std::uint32_t camera, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFacet(
        std::uint32_t actor) = 0;
    virtual void UpdatePosition(const DeadEffectHandle& effect,
                                std::uint32_t position,
                                std::uint8_t mode) = 0;
    virtual void UpdateScale(const DeadEffectHandle& effect,
                             const std::array<float, 3>& scale,
                             std::uint8_t mode) = 0;
    virtual void FinalizeEffect(const DeadEffectHandle& effect) = 0;
    virtual void ResetTemporaryEffect(DeadEffectHandle& effect) = 0;
};

class DeadEffectTask final {
public:
    static constexpr std::uint32_t kEffectName = 0x8b67d0u;
    static constexpr std::uint32_t kEffectPosition = 0x84b1dcu;

    // IDA 0x55b0f0.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IDeadEffectTaskRuntime& runtime);
};

}  // namespace pal4
