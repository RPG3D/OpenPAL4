#pragma once

#include "pal4/actor_animation_events.h"
#include "pal4/actor_render_state.h"
#include "pal4/actor_resource_loader.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

struct ActorAttachmentLoadArgs {
    std::string_view model_name;
    std::string_view texture_name;
    std::string_view dialog_text;
    std::string_view primary_text;
    std::string_view secondary_text;
    std::uint32_t animation_id = 0;
    bool alternate_resource = false;
};

class IActorAnimationSearchRuntime : public IActorAnimationEventRuntime {
public:
    [[nodiscard]] virtual bool AnimationFound() = 0;
    virtual void ProcessAnimations(
        std::uint32_t state, std::uint32_t search,
        const std::function<std::uint32_t(std::uint32_t,
                                          std::uint32_t)>& visitor) = 0;
};

class IActorCameraCleanupRuntime {
public:
    virtual ~IActorCameraCleanupRuntime() = default;
    virtual void CleanupCamera(std::uint32_t actor) = 0;
};

class IActorAnimationControlRuntime : public IActorCameraCleanupRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int64_t FloatToInteger(float value) = 0;
    virtual void InvokeActorSlot24(std::uint32_t actor,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessClumpAtomics(
        std::uint32_t clump, std::uint32_t queue) = 0;
    virtual void RenderCamera(std::uint32_t actor,
                              std::uint32_t argument) = 0;
};

class ActorAnimationControl final {
public:
    [[nodiscard]] static bool FindAnimation(std::uint32_t actor,
                                            std::uint32_t search,
                                            IActorAnimationSearchRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetAlpha(
        std::uint32_t actor, float alpha, std::uint32_t queue,
        IActorAnimationControlRuntime& runtime);
    // Both loaders take the whole ActorInstance receiver. They retain its
    // body clump as the hook context and load into the embedded hand image.
    [[nodiscard]] static std::uint32_t LoadRightHand(
        std::uint32_t actor, const ActorAttachmentLoadArgs& args,
        IActorResourceRuntime& runtime);
    [[nodiscard]] static std::uint32_t RightHandClump(
        std::uint32_t actor, IActorAnimationControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadLeftHand(
        std::uint32_t actor, const ActorAttachmentLoadArgs& args,
        IActorResourceRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeftHandClump(
        std::uint32_t actor, IActorAnimationControlRuntime& runtime);
    static void Activate(
        std::uint32_t actor, std::uint32_t camera_argument,
        std::uint32_t atomic_callback, std::uint32_t effect_context,
        IActorAnimationControlRuntime& runtime,
        IActorRenderStateRuntime& render_runtime);
    static void Deactivate(std::uint32_t actor,
                           std::uint32_t effect_context,
                           IActorCameraCleanupRuntime& runtime,
                           IActorAttachmentCleanupRuntime& render_runtime);
    static void SetLeftHandActive(std::uint32_t actor, std::uint8_t active,
                                  std::uint32_t effect_context,
                                  IActorRenderStateRuntime& runtime);
    static void SetRightHandActive(std::uint32_t actor, std::uint8_t active,
                                   std::uint32_t effect_context,
                                   IActorRenderStateRuntime& runtime);
};

}  // namespace pal4
