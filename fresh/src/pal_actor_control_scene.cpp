#include "pal4/pal_actor_control_scene.h"
#include "pal4/pal_actor_camera_tail_yaw.h"
#include "pal4/pal_actor_control_transform.h"

#include <algorithm>
#include <array>
#include <initializer_list>

namespace pal4 {

std::string_view IPalActorControlSceneRuntime::ActorName(
    std::uint32_t) { return {}; }
void IPalActorControlSceneRuntime::InitializeCameraSubsystem() {}
std::uint32_t IPalActorControlSceneRuntime::GetSceneCamera() {
    return 0u;
}
void IPalActorControlSceneRuntime::SetSceneParameter(
    std::uint32_t, std::uint32_t) {}
void IPalActorControlSceneRuntime::SetCameraLookAt(
    std::uint32_t, std::uint32_t) {}
void IPalActorControlSceneRuntime::AddMinimapIcon(
    std::string_view, std::string_view) {}
void IPalActorControlSceneRuntime::SetActiveMinimapIcon(
    std::string_view) {}
void IPalActorControlSceneRuntime::FlushMainCameraTailYaw(
    PalActorControlState& state) {
    PalActorControlScene::FlushTailYaw(state, *this);
}
std::uint32_t IPalActorControlSceneRuntime::GetCameraInternal(
    std::uint32_t) { return 0u; }
float IPalActorControlSceneRuntime::ComputeYawCorrection(
    PalActorControlState& state, std::uint32_t camera_internal,
    std::uint32_t actor, float& signed_component) {
    ActorControlTransformRuntimeStub runtime;
    return PalActorControlTransform::Compute(state, camera_internal,
                                             actor, &signed_component,
                                             runtime);
}
float IPalActorControlSceneRuntime::CurrentCameraYaw(
    std::uint32_t) { return 0.0f; }
void IPalActorControlSceneRuntime::SetCameraYaw(
    std::uint32_t, float, std::uint32_t) {}
void IPalActorControlSceneRuntime::BindSubobject(
    std::uint32_t*, std::uint32_t, std::uint32_t) {}
void IPalActorControlSceneRuntime::AddOrReplaceAnimationMember(
    std::uint32_t, std::uint32_t*) {}
void IPalActorControlSceneRuntime::SetMonsterStateParameter(
    std::uint32_t) {}
std::uint32_t IPalActorControlSceneRuntime::BottomHookAtomic(
    std::uint32_t, std::string_view) { return 0u; }
void IPalActorControlSceneRuntime::SetModeParameters(
    std::uint32_t*, std::uint32_t, std::uint32_t) {}
std::uint32_t IPalActorControlSceneRuntime::FindMovementFrame() {
    return 0u;
}
void IPalActorControlSceneRuntime::UpdateCharacterMovementState(
    std::uint32_t) {}
void IPalActorControlSceneRuntime::UpdateGameStateConditional(
    PalActorControlState& state, std::uint32_t value) {
    PalActorControlScene::SetModeConditional(state, value, *this);
}

void PalActorControlScene::SetActor(
    PalActorControlState& state, std::uint32_t actor,
    IPalActorControlSceneRuntime& runtime) {
    if (actor == 0u)
        return;

    auto& words = state.words;
    if (words[1] != 0u && words[33] == 2u)
        runtime.SetActorAnimationFlag(words[1], 0x3f800000u);
    words[1] = actor;

    // The original keeps the previous index when none of these names match.
    constexpr std::array<std::string_view, 4> kNames{
        "Player-TIANHE", "Player-LINSHA", "Player-MENGLI",
        "Player-ZIYING"};
    const auto name = runtime.ActorName(actor);
    for (std::uint32_t index = 0u; index < kNames.size(); ++index) {
        if (std::equal(name.begin(), name.end(), kNames[index].begin(),
                       kNames[index].end(),
                       [](char a, char b) {
                           if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
                           if (b >= 'a' && b <= 'z') b -= 'a' - 'A';
                           return a == b;
                       })) {
            words[19] = index;
            break;
        }
    }

    const auto pet = runtime.GetPetManager();
    if (runtime.PetActive(pet)) {
        runtime.ScriptPetOnRemove(runtime.GetArenaManager());
        runtime.PetVirtualRemove(runtime.GetPetManager());
        static_cast<void>(runtime.ResetPet(runtime.GetPetManager()));
        if (words[19] == 0u) {
            runtime.PetVirtualLoad(runtime.GetPetManager(),
                                   "PAL4Pet", "339", 1u);
            runtime.InitializePetData(runtime.GetPetManager(), actor);
            runtime.SetPetStateFlag(runtime.GetPetManager(), 1u);
            static_cast<void>(runtime.ScriptPetOnAdd(
                runtime.GetArenaManager()));
        }
    }

    runtime.InitializeCameraSubsystem();
    const auto camera = runtime.GetSceneCamera();
    runtime.SetSceneParameter(camera, actor + 0x1ecu);
    runtime.SetCameraLookAt(camera, 1u);
    runtime.AddMinimapIcon("LeadingRole", "actor.png");
    runtime.SetActiveMinimapIcon("LeadingRole");
    runtime.FlushMainCameraTailYaw(state);

    for (const auto index : {7u, 8u, 183u, 5u, 20u, 21u, 22u})
        words[index] = 0u;
    words[3] = 1u;
    words[4] = 1u;
    for (const auto index : {23u, 35u, 165u})
        runtime.BindSubobject(words.data() + index, words[1], 16u);
    for (const auto index : {209u, 216u})
        runtime.BindSubobject(words.data() + index, words[1], 12u);
    for (const auto index : {205u, 207u, 209u, 216u})
        runtime.AddOrReplaceAnimationMember(
            words[1] + 0x170u, words.data() + index);
    runtime.UpdateGameStateConditional(state, words[33]);
    runtime.SetMonsterStateParameter(actor);
    runtime.UpdateGameStateAndUi(state, words[1]);
    words[223] = runtime.BottomHookAtomic(actor, "hook_ef_bottom");
    words[224] = 0u;
}

void PalActorControlScene::FlushTailYaw(
    PalActorControlState& state,
    IPalActorControlSceneRuntime& runtime) {
    FlushActorCameraTailYaw(state, runtime);
}

void PalActorControlScene::SetModeConditional(
    PalActorControlState& state, std::uint32_t mode,
    IPalActorControlSceneRuntime& runtime) {
    const auto actor = state.words[1];
    if (actor == 0u)
        return;
    if (state.words[195] != 0u) {
        runtime.SetModeParameters(state.words.data() + 23u, 0u, actor);
        const auto frame = runtime.FindMovementFrame();
        if (frame != 0u)
            runtime.UpdateCharacterMovementState(frame);
    } else {
        runtime.SetModeParameters(state.words.data() + 23u, mode, actor);
        runtime.UpdateGameStateAndUi(state, state.words[1]);
    }
}

}  // namespace pal4
