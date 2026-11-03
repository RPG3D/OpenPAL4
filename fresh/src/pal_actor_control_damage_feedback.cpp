#include "pal4/pal_actor_control_damage_feedback.h"

#include <bit>
#include <cmath>
#include <limits>
#include <string>

namespace pal4 {

void IActorControlDamageFeedbackAnimationRuntime::UpdateAnimationProgress(
    float) {}
std::uint16_t IActorControlDamageFeedbackAnimationRuntime::FpuCompareStatus(
    const long double left, const long double right) {
    if (std::isnan(left) || std::isnan(right)) {
        return 0x4500u;
    }
    if (left < right) {
        return 0x0100u;
    }
    return left == right ? 0x4000u : 0u;
}
void IActorControlDamageFeedbackDeleteRuntime::Cleanup(std::uint32_t) {}
void IActorControlDamageFeedbackDeleteRuntime::Free(std::uint32_t) {}

std::uint32_t IActorControlDamageFeedbackRuntime::PlayerManager() { return 0; }
std::uint32_t IActorControlDamageFeedbackRuntime::FindPlayer(
    std::uint32_t, std::uint32_t) { return 0; }
std::int32_t IActorControlDamageFeedbackRuntime::ReadAttribute(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0; }
void IActorControlDamageFeedbackRuntime::ChangeAttribute(
    std::uint32_t, std::uint32_t, std::int32_t) {}
std::int32_t IActorControlDamageFeedbackRuntime::FloatToLong(
    const long double value) {
    if (!std::isfinite(value) ||
        value < static_cast<long double>(std::numeric_limits<std::int32_t>::min()) ||
        value > static_cast<long double>(std::numeric_limits<std::int32_t>::max())) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(value);
}
DamageWorldPoint IActorControlDamageFeedbackRuntime::PlayerPosition(
    std::uint32_t) { return {}; }
float IActorControlDamageFeedbackRuntime::PlayerHeight(std::uint32_t) { return 0; }
std::uint32_t IActorControlDamageFeedbackRuntime::CameraSubsystem() { return 0; }
std::uint32_t IActorControlDamageFeedbackRuntime::ActiveCamera(
    std::uint32_t) { return 0; }
DamageScreenPoint IActorControlDamageFeedbackRuntime::Project(
    std::uint32_t, const DamageWorldPoint&) { return {}; }
std::uint32_t IActorControlDamageFeedbackRuntime::UiFrameManager() { return 0; }
DamageScreenPoint IActorControlDamageFeedbackRuntime::ConvertCoordinates(
    std::uint32_t, const DamageScreenPoint& point) { return point; }
std::uint32_t IActorControlDamageFeedbackRuntime::Allocate(std::uint32_t) {
    return 0;
}
void IActorControlDamageFeedbackRuntime::InitializeFeedback(std::uint32_t) {}
void IActorControlDamageFeedbackRuntime::WriteWord(
    std::uint32_t, std::uint32_t) {}
std::string IActorControlDamageFeedbackRuntime::FormatDamage(
    const std::int32_t amount) { return std::to_string(amount); }
void IActorControlDamageFeedbackRuntime::InitializeFont(
    std::uint32_t, std::string_view, std::string_view, float) {}
void IActorControlDamageFeedbackRuntime::SetPosition(
    std::uint32_t, const DamageScreenPoint&) {}
void IActorControlDamageFeedbackRuntime::SetFontProperties(
    std::uint32_t, float, float) {}
void IActorControlDamageFeedbackRuntime::AddFeedback(
    std::uint32_t, std::uint8_t, std::uint32_t) {}
std::uint32_t IActorControlDamageFeedbackRuntime::CleanupText(
    std::string&) { return 0; }

std::uint32_t PalActorControlDamageFeedback::Apply(
    const PalActorControlState& state, const float percentage,
    IActorControlDamageFeedbackRuntime& runtime) {
    const auto index = state.words[19u];
    if (index >= 4u) {
        return index;
    }
    const auto player = runtime.FindPlayer(runtime.PlayerManager(), index);
    if (player == 0u) {
        return 0u;
    }
    const auto attributes = player + 0x654u;
    const auto current = runtime.ReadAttribute(attributes, 0x34u, 7u);
    if (current <= 1) {
        return std::bit_cast<std::uint32_t>(current);
    }

    const auto base = runtime.ReadAttribute(attributes, 0x30u, 0u);
    auto damage = runtime.FloatToLong(
        static_cast<long double>(base) *
        static_cast<long double>(0.0099999998f) *
        static_cast<long double>(percentage));
    if (damage <= 0) {
        damage = 1;
    }
    const auto negative_damage = -damage;
    runtime.ChangeAttribute(attributes, 7u, negative_damage);
    if (runtime.ReadAttribute(attributes, 0x34u, 7u) <= 0) {
        runtime.ChangeAttribute(attributes, 7u, 1);
    }

    auto world = runtime.PlayerPosition(player + 0xacu);
    world[1] += runtime.PlayerHeight(player + 0xf4u) + 20.0f;
    const auto first_subsystem = runtime.CameraSubsystem();
    const auto second_subsystem = runtime.CameraSubsystem();
    const auto first_camera = runtime.ActiveCamera(second_subsystem);
    const auto first_projection = runtime.Project(first_camera, world);
    const auto second_camera = runtime.ActiveCamera(first_subsystem);
    const auto second_projection = runtime.Project(second_camera, world);
    const DamageScreenPoint mixed{second_projection[0], first_projection[1]};
    const auto screen = runtime.ConvertCoordinates(
        runtime.UiFrameManager(), mixed);

    const auto object = runtime.Allocate(0x5cu);
    if (object != 0u) {
        runtime.InitializeFeedback(object);
        runtime.WriteWord(object, 0x842698u);
        for (const auto offset : {0x58u, 0x50u, 0x54u, 0x48u, 0x4cu}) {
            runtime.WriteWord(object + offset, 0u);
        }
    }

    auto text = runtime.FormatDamage(negative_damage);
    try {
        runtime.InitializeFont(object, text, "RoleHpDec", 1.0f);
        runtime.SetPosition(object, screen);
        runtime.SetFontProperties(object, 0.5f, 0.5f);
        runtime.WriteWord(object + 0x50u, std::bit_cast<std::uint32_t>(0.1f));
        runtime.WriteWord(object + 0x54u, std::bit_cast<std::uint32_t>(0.1f));
        runtime.WriteWord(object + 0x48u, std::bit_cast<std::uint32_t>(5.0f));
        runtime.WriteWord(object + 0x4cu, std::bit_cast<std::uint32_t>(5.0f));
        runtime.WriteWord(object + 0x44u, std::bit_cast<std::uint32_t>(80.0f));
        runtime.WriteWord(object + 0x58u, 0u);
        runtime.AddFeedback(player + 0xaa8u, 1u, object);
    } catch (...) {
        (void)runtime.CleanupText(text);
        throw;
    }
    return runtime.CleanupText(text);
}

std::uint32_t PalActorControlDamageFeedback::TickAnimation(
    DamageFeedbackRecord& record, const float delta,
    IActorControlDamageFeedbackAnimationRuntime& runtime) {
    runtime.UpdateAnimationProgress(delta);
    auto& words = record.words;
    const auto timer = std::bit_cast<float>(words[22u]);
    if ((runtime.FpuCompareStatus(timer, 0.0L) & 0x0100u) == 0u) {
        const auto elapsed = static_cast<long double>(delta) + timer;
        words[22u] = std::bit_cast<std::uint32_t>(
            static_cast<float>(elapsed));
        const auto value = static_cast<long double>(delta) *
            std::bit_cast<float>(words[18u]) +
            std::bit_cast<float>(words[10u]);
        words[10u] = std::bit_cast<std::uint32_t>(
            static_cast<float>(value));
        words[9u] = words[10u];
        const auto status = runtime.FpuCompareStatus(
            elapsed, std::bit_cast<float>(words[20u]));
        const auto result = static_cast<std::uint32_t>(status & 0x4100u);
        if (result == 0u) {
            words[22u] = 0xb8d1b717u;
        }
        return result;
    }

    const auto status = runtime.FpuCompareStatus(
        -static_cast<long double>(timer),
        std::bit_cast<float>(words[21u]));
    if ((status & 0x0500u) != 0x0100u) {
        words[9u] = words[10u];
        return words[10u];
    }
    words[22u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(static_cast<long double>(timer) - delta));
    const auto value = static_cast<long double>(
        std::bit_cast<float>(words[10u])) -
        static_cast<long double>(delta) *
            std::bit_cast<float>(words[19u]);
    words[10u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(value));
    words[9u] = words[10u];
    return status;
}

std::uint32_t PalActorControlDamageFeedback::Delete(
    const std::uint32_t object, const std::uint32_t flags,
    IActorControlDamageFeedbackDeleteRuntime& runtime) {
    runtime.Cleanup(object);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

}  // namespace pal4
