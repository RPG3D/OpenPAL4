#include "pal4/combat_console_number_events.h"

#include "pal4/animation_event_queue.h"
#include "pal4/combat_camera_access.h"
#include "pal4/ui_text_image_animation.h"
#include "pal4/ui_text_image_overlay.h"
#include "combat_console_activity_owner.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {

namespace {

std::uint32_t BitsOf(const std::int32_t value) {
    return std::bit_cast<std::uint32_t>(value);
}

std::int32_t SignedOf(const std::uint32_t value) {
    return std::bit_cast<std::int32_t>(value);
}

std::uint32_t CreateNumber(
    const std::uint32_t event, const std::uint32_t actor,
    const std::uint32_t property, const std::int32_t amount,
    const bool negative,
    const std::array<std::uint32_t, 2>& position,
    ICombatConsoleNumberRuntime& runtime) {
    const auto allocation = runtime.Allocate(CombatConsoleNumberEvents::kElementSize);
    if (allocation == 0u) return 0u;

    detail::CombatConsoleActivityOwner owner(allocation, runtime);
    const auto element = UiTextImageAnimation::ConstructMotion(allocation, 0u,
                                                              runtime);
    owner.Constructed();
    runtime.WriteWord(element, CombatConsoleNumberEvents::kVtable);
    for (const auto offset : {88u, 80u, 84u, 72u, 76u})
        runtime.WriteWord(element + offset, 0u);

    const std::string text = (negative ? "-" : "+") +
                             std::to_string(amount);
    try {
        const std::string_view font = negative ?
            (runtime.ReadWord(actor + 0x324u) == 1u ?
                "RoleHpDec" : "MonsterHpDec") :
            (property == 7u ? "HpInc" : "MpInc");
        (void)UiTextImageOverlay::SetTextAndFont(
            element, text, font, 0x3f800000u, runtime);
        (void)UiTextImageOverlay::SetPosition(element, position[0],
                                              position[1], runtime);
        (void)UiTextImageAnimation::SetTiming(element, 0x3f000000u,
                                              0x3fc00000u, runtime);
        runtime.WriteWord(element + 80u, 0x3dcccccdu);
        runtime.WriteWord(element + 84u, 0x3dcccccdu);
        runtime.WriteWord(element + 68u, 0x42a00000u);
        runtime.WriteWord(element + 72u, 0x40a00000u);
        runtime.WriteWord(element + 76u, 0x40a00000u);
        runtime.WriteWord(element + 88u, 0u);
        const auto queue_actor = runtime.DialogData(event);
        (void)runtime.EnqueueActorAnimation(queue_actor,element,true);
        owner.Release();
    } catch (...) {
        (void)runtime.CleanupFormattedText(text);
        throw;
    }
    return runtime.CleanupFormattedText(text);
}

}  // namespace

std::uint32_t CombatConsoleNumberEvents::Display(
    const std::uint32_t event, ICombatConsoleNumberRuntime& runtime) {
    const auto before = runtime.ReadWord(event + 8u);
    const auto delta = SignedOf(runtime.ReadWord(event + 12u) - before);
    const auto actor = runtime.DialogData(event);
    const auto property = runtime.ReadWord(event + 4u);
    if (property != 7u && (property != 9u || delta <= 0))
        return property;

    std::int32_t amount = 0;
    bool negative = false;
    if (property == 7u && delta <= 0) {
        amount = SignedOf(0u - BitsOf(delta));
        negative = true;
    } else {
        amount = SignedOf(runtime.ActorAttribute(actor, property) - before);
    }

    runtime.EnsureCombatSystem();
    const auto camera = CombatCameraAccess::Current(runtime);
    const auto projection_actor = property == 7u && delta > 0 ?
                                      runtime.DialogData(event) : actor;
    std::array<std::uint32_t, 2> position{};
    const auto projection_result = runtime.ActorScreenPosition(
        projection_actor, camera, position);
    if (property == 7u && delta > 0 && amount <= 0)
        return projection_result;
    return CreateNumber(event, actor, property, amount, negative,
                        position, runtime);
}

}  // namespace pal4
