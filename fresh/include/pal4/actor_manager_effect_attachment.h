#pragma once

#include "pal4/actor_render_state.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerEffectAttachmentRuntime {
public:
    virtual ~IActorManagerEffectAttachmentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ChildClump(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CombatEffectGroup() = 0;
    [[nodiscard]] virtual ActorEffectHandle CreateEffect(
        std::uint32_t camera, std::string_view resource,
        const std::array<float, 3>& initial_position) = 0;
    [[nodiscard]] virtual bool IsValid(ActorEffectHandle effect) = 0;
    [[nodiscard]] virtual bool MatchesPrimaryMode(std::string_view mode) = 0;
    [[nodiscard]] virtual bool MatchesSecondaryMode(std::string_view mode) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorSlot448Position(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::array<float, 3> ChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ChildRotationVector(std::uint32_t actor) = 0;
    virtual void SetVectorProperty(ActorEffectHandle effect,
                                   const std::array<float, 3>& value,
                                   bool enabled, std::uint32_t slot) = 0;
    virtual void SetRotationVectorProperty(ActorEffectHandle effect,
                                   std::uint32_t vector,
                                   bool enabled) = 0;
    virtual void SetFacetProperty(ActorEffectHandle effect,
                                 std::string_view text,
                                 std::uint32_t actor_field) = 0;
    [[nodiscard]] virtual std::uint32_t EffectFrame(
        ActorEffectHandle effect) = 0;
    virtual void ProcessClumpUserData(std::uint32_t clump,
                                      std::string_view mode,
                                      std::uint32_t data) = 0;
    virtual void EnableTasks(ActorEffectHandle effect) = 0;
    [[nodiscard]] virtual ActorEffectHandle CopyEffect(
        ActorEffectHandle effect) = 0;
    virtual void DispatchAttachEffect(std::uint32_t actor,
                                      ActorEffectHandle effect) = 0;
    virtual void ResetTemporary(ActorEffectHandle effect) = 0;
    virtual void ResetHandleAt(std::uint32_t address) = 0;
};

class ActorManagerEffectAttachment final {
public:
    [[nodiscard]] static ActorEffectHandle Create(
        std::uint32_t actor, std::string_view resource,
        std::string_view mode, IActorManagerEffectAttachmentRuntime& runtime);
    static void ResetEmbedded(std::uint32_t object,
                              IActorManagerEffectAttachmentRuntime& runtime);
};

}  // namespace pal4
