#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct CombatEffectHandle {
    std::uint32_t object = 0;
    std::uint32_t generation = 0;
};

class ICombatAnimationCommandRuntime {
public:
    virtual ~ICombatAnimationCommandRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Actor(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t FindAnimationEntry(
        std::uint32_t actor, std::string_view original_name) = 0;
    [[nodiscard]] virtual std::uint32_t ChildClump(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string Attribute(
        std::uint32_t entry, std::string_view key) = 0;
    [[nodiscard]] virtual float FloatAttribute(
        std::uint32_t entry, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual int IntAttribute(
        std::uint32_t entry, std::string_view key, int fallback) = 0;
    [[nodiscard]] virtual bool EffectActive(CombatEffectHandle handle) = 0;
    [[nodiscard]] virtual bool SlotActive(std::uint32_t slot) = 0;
    virtual void ReleaseSlot(std::uint32_t slot) = 0;
    [[nodiscard]] virtual CombatEffectHandle CreateEffect(
        std::string_view resource) = 0;
    virtual void StoreSlot(std::uint32_t slot, CombatEffectHandle handle,
                           std::string_view bind) = 0;
    virtual void SetCallback(CombatEffectHandle handle,
                             std::string_view property, std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t TaskTarget(std::uint32_t task) = 0;
    virtual void SetTarget(CombatEffectHandle handle,
                           std::string_view property, std::uint32_t target) = 0;
    virtual void AttachActorTransform(CombatEffectHandle handle,
                                      std::uint32_t actor) = 0;
    // Original additive Euler rotation(90,0,0) before hook attachment.
    virtual void ApplyBindingRotation(CombatEffectHandle handle) = 0;
    [[nodiscard]] virtual std::uint32_t EffectFrame(
        CombatEffectHandle handle) = 0;
    virtual void ProcessBinding(std::uint32_t clump, std::string_view bind,
                                std::uint32_t frame) = 0;
    virtual void UpdateEffect(CombatEffectHandle handle) = 0;
    virtual void ResetTemporaryEffect(CombatEffectHandle handle) = 0;
    virtual void SetTimeScale(float scale) = 0;
    virtual void SetHandsActive(std::uint32_t actor, bool active) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupCommand(
        std::string_view command) = 0;
};

class CombatAnimationCommandProcessor final {
public:
    // IDA 0x556bb0. Negative slot indices are rejected at this C++ boundary;
    // the original checks only index < 5 and would address before the object.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t task, std::string_view message_name,
        ICombatAnimationCommandRuntime& runtime);
};

}  // namespace pal4
