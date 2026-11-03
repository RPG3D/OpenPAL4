#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorEffectHandle {
    std::uint32_t id = 0;
    std::uint32_t owner = 0;
};

// Cleanup needs no effect creation, camera lookup or rendering services.
class IActorAttachmentCleanupRuntime {
public:
    virtual ~IActorAttachmentCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void RemoveEffect(std::uint32_t handle) = 0;
    virtual void HandleEvent(std::uint32_t handle,
                             std::uint32_t context, bool force) = 0;
    // Operation boundary lets native owners pin a hand for callbacks while
    // the default preserves the shared52A2E0 inactive algorithm.
    virtual void DeactivateAttachment(std::uint32_t actor,std::uint32_t context);
};

class IActorRenderStateRuntime : public IActorAttachmentCleanupRuntime {
public:
    virtual void SetAttachmentState(std::uint32_t actor,std::uint8_t active,
                                    std::uint32_t context);
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view TextAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatCamera() = 0;
    [[nodiscard]] virtual ActorEffectHandle LoadXmlEffect(
        std::uint32_t camera, std::string_view name,
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual ActorEffectHandle CreateEffect(
        std::uint32_t camera, std::string_view name) = 0;
    virtual void ResetTemporaryEffect() = 0;
    virtual void ConfigureEffect(
        std::uint32_t handle, const std::array<float, 3>& rotation,
        bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t EffectFrame(
        std::uint32_t handle) = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
    virtual void UpdateEffectProperties(std::uint32_t handle) = 0;
    virtual void TouchSkillSystem() = 0;
    [[nodiscard]] virtual bool DialogNeedsUpdate(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool IsReadyToShow(
        std::uint32_t handle) = 0;
};

class ActorRenderStateUpdater final {
public:
    // Inactive branch of52A2E0; preserves handles when already inactive.
    static void Deactivate(std::uint32_t actor, std::uint32_t effect_context,
                           IActorAttachmentCleanupRuntime& runtime);
    // IDA 0x529f90; the native EffectSystem/RenderWare calls are injected.
    static void Update(std::uint32_t actor,
                       IActorRenderStateRuntime& runtime);
    static void SetActiveState(std::uint32_t actor, std::uint8_t active,
                               std::uint32_t effect_context,
                               IActorRenderStateRuntime& runtime);
};

class IActorClumpRenderRuntime {
public:
    virtual ~IActorClumpRenderRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void RenderClump(std::uint32_t clump) = 0;
    virtual void RenderCamera(std::uint32_t camera,
                              std::uint32_t argument) = 0;
};

class ActorClumpRenderer final {
public:
    // IDA 0x52a2c0 jumps to the shared rendering tail at 0x444810.
    static void Render(std::uint32_t actor, std::uint32_t argument,
                       IActorClumpRenderRuntime& runtime);
};

}  // namespace pal4
