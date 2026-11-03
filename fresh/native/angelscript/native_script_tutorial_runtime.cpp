#include "pal4/native_script_tutorial_runtime.h"
#include <stdexcept>
#include <utility>

namespace pal4 {
NativeScriptTutorialRuntime::NativeScriptTutorialRuntime(asIScriptContext& context,
    NativeTutorialServices services) : context_(context), services_(std::move(services)) {
    if (!services_.show || !services_.frame || !services_.visible)
        throw std::invalid_argument("tutorial UI services missing");
}
std::uint32_t NativeScriptTutorialRuntime::UiFrameManager() { return reinterpret_cast<std::uint32_t>(this); }
std::uint32_t NativeScriptTutorialRuntime::ScriptExecutor() { return UiFrameManager(); }
void NativeScriptTutorialRuntime::ShowTutorial(std::uint32_t manager, std::uint32_t key) {
    if (manager != UiFrameManager() || active_) throw std::logic_error("tutorial owner is busy");
    services_.show(key);
    ++requests_;
    last_key_ = key;
}
std::uint32_t NativeScriptTutorialRuntime::FindFrame(std::uint32_t manager, std::string_view name) {
    if (manager != UiFrameManager() || name != ScriptTutorialWait::kFrameName)
        throw std::invalid_argument("tutorial frame lookup");
    return services_.frame();
}
TutorialWaitState* NativeScriptTutorialRuntime::AllocateWait(std::uint32_t bytes) {
    if (bytes != sizeof(state_) || active_) throw std::logic_error("tutorial wait allocation");
    state_ = {};
    return &state_;
}
void NativeScriptTutorialRuntime::ConstructWait(TutorialWaitState& state, std::uint8_t mode, std::uint32_t queue) {
    if (&state != &state_ || mode != 1u || queue != ScriptExecutor() + ScriptTutorialWait::kQueueOffset)
        throw std::logic_error("tutorial wait queue");
}
void NativeScriptTutorialRuntime::FlushWait(TutorialWaitState& state) {
    if (&state != &state_ || state.vtable_address != ScriptTutorialWait::kWaitVtable)
        throw std::logic_error("tutorial wait vtable");
}
std::uint32_t NativeScriptTutorialRuntime::AddHandler(std::uint32_t queue, TutorialWaitState* state) {
    if (queue != ScriptExecutor() + ScriptTutorialWait::kQueueOffset || state != &state_ ||
        active_ || asGetActiveContext() != &context_ || context_.Suspend() < 0)
        throw std::runtime_error("tutorial cannot suspend its SDK context");
    active_ = true;
    return 1u;
}
bool NativeScriptTutorialRuntime::IsVisible(std::uint32_t frame) { return services_.visible(frame); }
void NativeScriptTutorialRuntime::CloseWait(TutorialWaitState& state) {
    if (&state != &state_) throw std::logic_error("tutorial wait close owner");
    active_ = false;
}
bool NativeScriptTutorialRuntime::Tick(float) {
    const bool waiting = active_;
    if (active_) tutorial_.Update(state_);
    return waiting && !active_;
}
void NativeScriptTutorialRuntime::ClearHandlers() {
    if (asGetActiveContext() == &context_) throw std::logic_error("cannot retire active tutorial callback");
    active_ = false;
    state_ = {};
}
} // namespace pal4
