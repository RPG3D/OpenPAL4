#include "pal4/actor_manager_action.h"

#include <string>

namespace pal4 {

void ActorManagerAction::Play(
    const std::uint32_t actor, const std::string_view action,
    const std::uint8_t flag, IActorManagerActionRuntime& runtime) {
    auto child = runtime.ReadWord(actor + 808u);
    if (!child) return;
    std::string animation(action);
    animation += ".anm";
    if (!runtime.FindAnimation(child, animation)) {
        runtime.LoadMissingAnimation(actor);
        // Player slot568 may replace actor808 (IDA52BABF onward reloads it).
        child = runtime.ReadWord(actor + 808u);
    }
    if (runtime.FindAnimation(child, animation)) {
        runtime.SetAttachmentActive(child, false, 1u, actor);
        runtime.SetAttachmentActive(child, true, 1u, actor);
    }
    runtime.SetAnimationFlag(child + 368u, flag);
    runtime.SetAnimationName(child + 368u, action);
    runtime.TouchSkillSystem();
}

std::uint32_t ActorManagerAction::Position(
    const std::uint32_t actor, IActorManagerActionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? child + 172u : kEmptyTransform;
}

bool ActorManagerAction::InvokeRotation(
    const std::uint32_t actor, const std::uint32_t first,
    const std::uint32_t second, IActorManagerActionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    if (!child) return false;
    runtime.InvokeChildSlot4(child, first, second);
    return true;
}

std::uint32_t ActorManagerAction::InvokeScale(
    const std::uint32_t actor, const std::uint32_t first,
    const std::uint32_t second, IActorManagerActionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? runtime.InvokeChildSlot8(child, first, second) : 0u;
}

std::uint32_t ActorManagerAction::Matrix(
    const std::uint32_t actor, IActorManagerActionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? child + 160u : kEmptyTransform;
}

std::uint32_t ActorManagerAction::BoundingBox(
    const std::uint32_t actor, IActorManagerActionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? child + 524u : kEmptyBoundingBox;
}

std::uint32_t ActorManagerAction::Clump(
    const std::uint32_t actor, IActorManagerChildRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? runtime.ReadWord(child + 20u) : 0u;
}

std::uint32_t ActorManagerAction::AnimationFlag(
    const std::uint32_t actor, IActorManagerAnimationQueryRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? runtime.GetAnimationFlag(child + 368u) : 1u;
}

std::string_view ActorManagerAction::AnimationName(
    const std::uint32_t actor, IActorManagerAnimationQueryRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? runtime.GetAnimationName(child + 368u) :
                   std::string_view{};
}

}  // namespace pal4
