#include "pal4/combat_console_image_events.h"

#include "pal4/ui_text_image_animation.h"
#include "combat_console_activity_owner.h"

namespace pal4 {

std::uint32_t ICombatConsoleVisualRuntime::EnqueueActorAnimation(
    std::uint32_t actor,std::uint32_t animation,bool staggered) {
    const auto queue=actor+CombatConsoleImageEvents::kActorQueueOffset;
    return staggered ? AnimationEventQueue::AddWithDelay(queue,1u,animation,*this) :
        AnimationEventQueue::Add(queue,1u,animation,*this);
}

std::uint32_t CombatConsoleImageEvents::CreateMissImage(
    const std::uint32_t event, ICombatConsoleVisualRuntime& runtime) {
    return CreateImage(event, false, "luokong", runtime);
}

std::uint32_t CombatConsoleImageEvents::CreateImmuneImage(
    const std::uint32_t argument, ICombatConsoleVisualRuntime& runtime) {
    return CreateImage(argument, true, "mianyi", runtime);
}

std::uint32_t CombatConsoleImageEvents::CreateCounterImage(
    const std::uint32_t argument, ICombatConsoleVisualRuntime& runtime) {
    return CreateImage(argument, true, "fangji", runtime);
}

std::uint32_t CombatConsoleImageEvents::CreateImage(
    const std::uint32_t argument, const bool from_dialog,
    const char* image_name, ICombatConsoleVisualRuntime& runtime) {
    runtime.EnsureCombatSystem();
    const auto camera = CombatCameraAccess::Current(runtime);
    const auto actor = from_dialog ? runtime.DialogData(argument) :
                                     runtime.ReadWord(argument + 4u);
    std::array<std::uint32_t, 2> position{};
    (void)runtime.ActorScreenPosition(actor, camera, position);

    const auto allocation = runtime.Allocate(kElementSize);
    if (allocation == 0u) return 0u;

    detail::CombatConsoleActivityOwner owner(allocation, runtime);
    const auto element = UiTextImageAnimation::ConstructMotion(allocation, 0u,
                                                              runtime);
    owner.Constructed();

    (void)UiTextImageOverlay::SetPosition(element, position[0], position[1],
                                          runtime);
    UiTextImageOverlay::SetImage(element, "zi0", image_name, 0x3f800000u,
                                runtime);
    (void)UiTextImageAnimation::SetTiming(element, 0x3f800000u,
                                          0x40000000u, runtime);
    const auto queue_actor = from_dialog ? runtime.DialogData(argument) :
                                           runtime.ReadWord(argument + 4u);
    const auto result = runtime.EnqueueActorAnimation(queue_actor,element,false);
    owner.Release();
    return result;
}

}  // namespace pal4
