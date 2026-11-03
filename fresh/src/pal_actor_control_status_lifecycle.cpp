#include "pal4/pal_actor_control_status_lifecycle.h"

#include <bit>

namespace {

long double AdvanceTimer(std::uint32_t& timer_bits, const float delta) {
    const auto sum = static_cast<long double>(
        std::bit_cast<float>(timer_bits)) +
        static_cast<long double>(delta);
    timer_bits = std::bit_cast<std::uint32_t>(static_cast<float>(sum));
    return sum;
}

bool Reached(const long double elapsed, const std::uint32_t limit_bits) {
    return elapsed >= static_cast<long double>(
        std::bit_cast<float>(limit_bits));
}

}  // namespace

namespace pal4 {

void IActorControlStatusLifecycleRuntime::SetModeConditional(
    PalActorControlState&, std::uint32_t) {}
std::uint32_t IActorControlStatusLifecycleRuntime::UiFrameManager() {
    return 0u;
}
std::uint32_t IActorControlStatusLifecycleRuntime::FindFrame(
    std::uint32_t, std::string_view) {
    return 0u;
}
void IActorControlStatusLifecycleRuntime::UpdateCharacterMovementState(
    std::uint32_t) {}
bool IActorControlStatusLifecycleRuntime::HandleNeedsUpdate(
    std::uint32_t*) {
    return false;
}
std::uint8_t IActorControlStatusLifecycleRuntime::UpdateHandle(
    std::uint32_t*) {
    return 0u;
}
void IActorControlStatusLifecycleRuntime::HandleDamageOrStatus(
    PalActorControlState&, float) {}
std::uint8_t IActorControlStatusLifecycleRuntime::UpdateEffectProperties(
    std::uint32_t*, std::uint32_t, std::uint32_t) {
    return 0u;
}

void PalActorControlStatusLifecycle::BeginMovement(
    PalActorControlState& state, const std::uint32_t value,
    IActorControlStatusLifecycleRuntime& runtime) {
    auto& words = state.words;
    if (words[195u] != 0u) {
        return;
    }
    words[195u] = 1u;
    words[196u] = value;
    words[197u] = 0u;
    words[198u] = 0u;
    words[199u] = 0u;
    words[200u] = 0u;
    words[201u] = PalActorControl::GetMode(state);
    runtime.SetModeConditional(state, 0u);
    const auto frame = runtime.FindFrame(
        runtime.UiFrameManager(), "portrait");
    if (frame != 0u) {
        runtime.UpdateCharacterMovementState(frame);
    }
}

std::uint8_t PalActorControlStatusLifecycle::Clear(
    PalActorControlState& state,
    IActorControlStatusLifecycleRuntime& runtime) {
    auto& words = state.words;
    words[188u] = 0u;
    words[195u] = 0u;
    words[202u] = 0u;
    auto* handle = words.data() + 203u;
    if (runtime.HandleNeedsUpdate(handle)) {
        return runtime.UpdateHandle(handle);
    }
    return 0u;
}

std::uint8_t PalActorControlStatusLifecycle::Tick(
    PalActorControlState& state, const float delta_seconds,
    IActorControlStatusLifecycleRuntime& runtime) {
    auto& words = state.words;
    if (words[188u] != 0u) {
        const auto first = AdvanceTimer(words[190u], delta_seconds);
        if (Reached(first, words[189u])) {
            words[188u] = 0u;
        }
        const auto second = AdvanceTimer(words[192u], delta_seconds);
        if (Reached(second, words[191u])) {
            words[192u] = 0u;
            if (words[188u] != 0u) {
                runtime.HandleDamageOrStatus(
                    state, std::bit_cast<float>(words[193u]));
            }
        }
    }

    auto* handle = words.data() + 203u;
    if (words[195u] != 0u) {
        const auto elapsed = AdvanceTimer(words[197u], delta_seconds);
        if (Reached(elapsed, words[196u])) {
            const auto mode = words[201u];
            words[195u] = 0u;
            runtime.SetModeConditional(state, mode);
            const auto frame = runtime.FindFrame(
                runtime.UiFrameManager(), "portrait");
            if (frame != 0u) {
                runtime.UpdateCharacterMovementState(frame);
            }
            if (runtime.HandleNeedsUpdate(handle)) {
                (void)runtime.UpdateHandle(handle);
            }
        }
    }
    if (!runtime.HandleNeedsUpdate(handle)) {
        return 0u;
    }
    const auto delta_bits = std::bit_cast<std::uint32_t>(delta_seconds) &
        0xffffff00u;
    return runtime.UpdateEffectProperties(
        handle, words[1u] + 0xacu, delta_bits);
}

}  // namespace pal4
