#include "pal4/script_flash_effect.h"

namespace pal4 {

std::uint32_t ScriptFlashEffect::FlashOutBlack(
    float duration, std::uint8_t hold, std::uint8_t wait_mode) {
    return Start(duration, {0u, 0u, 0u}, false, hold, wait_mode);
}

void ScriptFlashEffect::UpdateFlashOutBlack(FlashWaitState& state) {
    const auto effect = runtime_.ScreenEffect();
    if (runtime_.ReadWord(effect + 48u) != 0u) runtime_.Close(state);
}

std::uint32_t ScriptFlashEffect::FlashInBlack(
    float duration, std::uint8_t wait_mode) {
    return Start(duration, {0u, 0u, 0u}, true, 0u, wait_mode);
}

std::uint32_t ScriptFlashEffect::FlashOutWhite(
    float duration, std::uint8_t hold, std::uint8_t wait_mode) {
    return Start(duration, {255u, 255u, 255u}, false, hold, wait_mode);
}

std::uint32_t ScriptFlashEffect::FlashInWhite(
    float duration, std::uint8_t wait_mode) {
    return Start(duration, {255u, 255u, 255u}, true, 0u, wait_mode);
}

std::uint32_t ScriptFlashEffect::FlashOutRed(
    float duration, std::uint8_t hold, std::uint8_t wait_mode) {
    return Start(duration, {229u, 10u, 10u}, false, hold, wait_mode);
}

std::uint32_t ScriptFlashEffect::FlashInRed(
    float duration, std::uint32_t unused, std::uint8_t wait_mode) {
    static_cast<void>(unused);
    return Start(duration, {229u, 10u, 10u}, true, 0u, wait_mode);
}

std::uint32_t ScriptFlashEffect::Start(
    float duration, FlashColor color, bool fade_in,
    std::uint32_t hold, std::uint8_t wait_mode) {
    const auto effect = runtime_.ScreenEffect();
    const auto phase = runtime_.ReadWord(effect + 48u);
    if (phase == 0u) return phase;

    const auto active_effect = runtime_.ScreenEffect();
    runtime_.StartFade(active_effect, duration, fade_in ? 0u : hold,
                       color, fade_in ? 255.0f : 0.0f,
                       fade_in ? 0.0f : 255.0f,
                       fade_in ? 0.0f : 255.0f);
    auto* state = runtime_.Allocate(sizeof(FlashWaitState));
    if (state != nullptr) {
        runtime_.Construct(*state, wait_mode,
                           runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        runtime_.Flush(*state);
    }
    return runtime_.AddHandler(
        runtime_.ScriptExecutor() + kQueueOffset, state);
}

}  // namespace pal4
