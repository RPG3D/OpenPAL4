#include "pal4/actor_render_state.h"

namespace pal4 {
void IActorAttachmentCleanupRuntime::DeactivateAttachment(std::uint32_t actor,std::uint32_t context) {
    ActorRenderStateUpdater::Deactivate(actor,context,*this);
}
void IActorRenderStateRuntime::SetAttachmentState(std::uint32_t actor,std::uint8_t active,std::uint32_t context) {
    ActorRenderStateUpdater::SetActiveState(actor,active,context,*this);
}
namespace {

void StoreHandle(const std::uint32_t address,
                 const ActorEffectHandle handle,
                 IActorRenderStateRuntime& runtime) {
    runtime.WriteWord(address, handle.id);
    runtime.WriteWord(address + 4u, handle.owner);
}

std::string_view TextOrEmpty(const std::uint32_t address,
                             IActorRenderStateRuntime& runtime) {
    const auto pointer = runtime.ReadWord(address);
    return pointer ? runtime.TextAt(pointer) : std::string_view{};
}

}  // namespace

void ActorRenderStateUpdater::Update(
    const std::uint32_t actor, IActorRenderStateRuntime& runtime) {
    constexpr auto first_offset = 396u;
    constexpr auto second_offset = 388u;
    constexpr auto third_offset = 404u;
    const auto first = actor + first_offset;
    const auto second = actor + second_offset;
    const auto third = actor + third_offset;
    runtime.RemoveEffect(first);
    runtime.RemoveEffect(second);
    runtime.HandleEvent(third, second, false);

    const auto first_name = TextOrEmpty(actor + 432u, runtime);
    const auto first_clump = runtime.ReadWord(actor + 20u);
    const auto first_camera = runtime.CombatCamera();
    StoreHandle(first,
                runtime.LoadXmlEffect(first_camera, first_name, first_clump),
                runtime);
    const auto second_name = TextOrEmpty(actor + 416u, runtime);
    const auto second_clump = runtime.ReadWord(actor + 20u);
    const auto second_camera = runtime.CombatCamera();
    StoreHandle(second,
                runtime.LoadXmlEffect(second_camera, second_name,
                                      second_clump), runtime);

    const auto clump = runtime.ReadWord(actor + 20u);
    const auto frame_owner = clump ? runtime.ReadWord(clump + 4u) : 0u;
    const auto frame = frame_owner ? runtime.ReadWord(frame_owner + 4u) : 0u;
    if (frame) {
        const auto third_name = TextOrEmpty(actor + 448u, runtime);
        const auto third_camera = runtime.CombatCamera();
        StoreHandle(third, runtime.CreateEffect(third_camera, third_name),
                    runtime);
        runtime.ResetTemporaryEffect();
        runtime.ConfigureEffect(third, {90.0f, 0.0f, 0.0f}, true);
        if (runtime.EffectFrame(third)) {
            runtime.AddFrameChild(frame, runtime.EffectFrame(third));
            runtime.UpdateEffectProperties(third);
        } else {
            runtime.HandleEvent(third, second, false);
        }
    }

    runtime.TouchSkillSystem();
    (void)runtime.DialogNeedsUpdate(third);
    (void)runtime.IsReadyToShow(second);
    (void)runtime.IsReadyToShow(first);
}

void ActorRenderStateUpdater::SetActiveState(
    const std::uint32_t actor, const std::uint8_t active,
    const std::uint32_t effect_context,
    IActorRenderStateRuntime& runtime) {
    if (!active) { Deactivate(actor, effect_context, runtime); return; }
    if (runtime.ReadByte(actor + 460u) == active) return;
    if (active && runtime.ReadWord(actor + 20u)) {
        Update(actor, runtime);
    } else {
        runtime.RemoveEffect(actor + 396u);
        runtime.RemoveEffect(actor + 388u);
        runtime.HandleEvent(actor + 404u, effect_context, false);
    }
    runtime.WriteByte(actor + 460u, active);
}

void ActorRenderStateUpdater::Deactivate(
    const std::uint32_t actor, const std::uint32_t effect_context,
    IActorAttachmentCleanupRuntime& runtime) {
    if (!runtime.ReadByte(actor + 460u)) return;
    runtime.RemoveEffect(actor + 396u);
    runtime.RemoveEffect(actor + 388u);
    runtime.HandleEvent(actor + 404u, effect_context, false);
    runtime.WriteByte(actor + 460u, 0u);
}

void ActorClumpRenderer::Render(
    const std::uint32_t actor, const std::uint32_t argument,
    IActorClumpRenderRuntime& runtime) {
    if (!runtime.ReadByte(actor + 460u) ||
        !runtime.ReadWord(actor + 20u) ||
        !runtime.ReadWord(actor + 36u)) return;
    const auto camera_ready = [&] {
        return runtime.ReadWord(actor + 44u) &&
               runtime.ReadWord(actor + 52u) &&
               runtime.ReadWord(actor + 48u);
    };
    if (runtime.ReadWord(actor + 24u)) {
        runtime.WriteWord(actor + 28u, 1u);
        runtime.RenderClump(runtime.ReadWord(actor + 20u));
        if (camera_ready()) runtime.RenderCamera(actor + 56u, argument);
    } else if (camera_ready()) {
        runtime.RenderCamera(actor + 56u, argument);
    }
}

}  // namespace pal4
