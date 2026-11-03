#include "pal4/native_script_flash_runtime.h"

#include "pal4/effect_render_parameters.h"
#include "pal4/pal_screen_effect.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pal4 {

static_assert(sizeof(void*) == sizeof(std::uint32_t));

NativeScriptFlashRuntime::NativeScriptFlashRuntime(
    PalScreenEffectState& effect, IEffectRenderRuntime& render,
    asIScriptContext& context)
    : effect_(effect), render_(render), context_(context), flash_(*this) {}

NativeScriptFlashRuntime::~NativeScriptFlashRuntime() = default;

std::uint32_t NativeScriptFlashRuntime::ScreenEffect() {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&effect_));
}

std::uint32_t NativeScriptFlashRuntime::ReadWord(
    const std::uint32_t address) {
    if (address != ScreenEffect() + 48u)
        throw std::invalid_argument("unexpected PAL4 screen-effect field");
    return effect_.render.phase;
}

void NativeScriptFlashRuntime::StartFade(
    const std::uint32_t effect, const float duration,
    const std::uint32_t hold, const FlashColor color,
    const float alpha_start, const float alpha_end,
    const float alpha_on_finish) {
    if (effect != ScreenEffect() || effect_.render.texture == 0u)
        throw std::runtime_error("PAL4 screen effect is not initialized");
    if (!std::isfinite(duration))
        throw std::invalid_argument("PAL4 flash duration is not finite");
    (void)PalScreenEffect::StartFade(
        effect_, duration, hold, color.red, color.green, color.blue,
        alpha_start, alpha_end, alpha_on_finish);
    if (duration <= 0.0f) {
        effect_.render.alpha = alpha_on_finish;
        effect_.render.phase = 1u;
        if (hold == 0u) effect_.render.active = 0u;
    }
}

FlashWaitState* NativeScriptFlashRuntime::Allocate(const std::uint32_t bytes) {
    if (bytes != sizeof(FlashWaitState) || pending_)
        throw std::logic_error("invalid PAL4 flash wait allocation");
    pending_ = std::make_unique<FlashWaitState>();
    return pending_.get();
}

std::uint32_t NativeScriptFlashRuntime::ScriptExecutor() {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(this));
}

std::uint32_t NativeScriptFlashRuntime::QueueAddress() const {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(this)) +
        ScriptFlashEffect::kQueueOffset;
}

void NativeScriptFlashRuntime::Construct(
    FlashWaitState& state, const std::uint8_t wait_mode,
    const std::uint32_t queue) {
    if (&state != pending_.get() || queue != QueueAddress())
        throw std::logic_error("PAL4 flash wait queue mismatch");
    pending_wait_mode_ = wait_mode;
}

void NativeScriptFlashRuntime::Flush(FlashWaitState& state) {
    if (&state != pending_.get())
        throw std::logic_error("PAL4 flash wait is not pending");
    if (pending_wait_mode_ != 0u) {
        if (asGetActiveContext() != &context_ || context_.Suspend() < 0)
            throw std::runtime_error("PAL4 script context cannot suspend");
    }
}

void NativeScriptFlashRuntime::Close(FlashWaitState& state) {
    const auto found = std::find_if(handlers_.begin(), handlers_.end(),
        [&state](const Handler& handler) {
            return handler.state.get() == &state;
        });
    if (found == handlers_.end())
        throw std::logic_error("PAL4 flash wait is not registered");
    found->closed = true;
}

std::uint32_t NativeScriptFlashRuntime::AddHandler(
    const std::uint32_t queue, FlashWaitState* state) {
    if (queue != QueueAddress() || state != pending_.get() || !state)
        throw std::logic_error("PAL4 flash wait registration mismatch");
    handlers_.push_back({std::move(pending_), false, pending_wait_mode_ != 0u});
    return static_cast<std::uint32_t>(handlers_.size());
}

bool NativeScriptFlashRuntime::Tick(const float elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0.0f)
        throw std::invalid_argument("PAL4 flash frame time is invalid");
    if (!world_clock_) EffectRenderControl::AdvanceFade(effect_.render, elapsed, render_);
    return ProcessWaitHandlers();
}

bool NativeScriptFlashRuntime::BlocksScript() const {
    return std::any_of(handlers_.begin(), handlers_.end(), [](const Handler& handler) {
        return handler.blocking && !handler.closed;
    });
}

bool NativeScriptFlashRuntime::HandlerRuntime::Tick(float elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0.0f)
        throw std::invalid_argument("PAL4 flash handler time is invalid");
    return owner_.ProcessWaitHandlers();
}

bool NativeScriptFlashRuntime::ProcessWaitHandlers() {
    const bool had_handlers = Waiting();
    for (auto& handler : handlers_)
        flash_.UpdateFlashOutBlack(*handler.state);
    std::erase_if(handlers_, [](const Handler& handler) {
        return handler.closed;
    });
    return had_handlers && !Waiting();
}

void NativeScriptFlashRuntime::ClearHandlers() {
    if (asGetActiveContext() == &context_)
        throw std::logic_error("PAL4 cannot clear active flash handlers");
    for (auto& handler : handlers_) Close(*handler.state);
    handlers_.clear();
    pending_.reset();
    pending_wait_mode_ = 0u;
}

}  // namespace pal4
