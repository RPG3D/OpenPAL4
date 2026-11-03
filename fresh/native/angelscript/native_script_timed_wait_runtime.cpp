#include "pal4/native_script_timed_wait_runtime.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

static_assert(sizeof(void*) == sizeof(std::uint32_t));

namespace pal4 {

std::uint32_t NativeScriptTimedWaitRuntime::QueueAddress() const {
    return reinterpret_cast<std::uint32_t>(this) +
           ScriptWaitHandler::kQueueOffset;
}

ScriptWaitHandlerState* NativeScriptTimedWaitRuntime::Allocate(
    std::uint32_t size) {
    if (size != ScriptWaitHandler::kSize || pending_)
        throw std::logic_error("PAL4 timed wait allocation mismatch");
    pending_ = std::make_unique<ScriptWaitHandlerState>();
    return pending_.get();
}

std::uint32_t NativeScriptTimedWaitRuntime::ScriptExecutor() {
    return reinterpret_cast<std::uint32_t>(this);
}

void NativeScriptTimedWaitRuntime::ConstructBase(
    ScriptWaitHandlerState& state, std::uint32_t wait_mode,
    std::uint32_t queue_address) {
    if (&state != pending_.get() || wait_mode != 1u ||
        queue_address != QueueAddress())
        throw std::logic_error("PAL4 timed wait queue mismatch");
}

void NativeScriptTimedWaitRuntime::Flush(ScriptWaitHandlerState& state) {
    if (&state != pending_.get())
        throw std::logic_error("PAL4 timed wait handler is not pending");
}

void NativeScriptTimedWaitRuntime::AddHandler(
    std::uint32_t queue_address, ScriptWaitHandlerState* state) {
    if (queue_address != QueueAddress() || state != pending_.get() || !state)
        throw std::logic_error("PAL4 timed wait registration mismatch");
    handlers_.push_back({std::move(pending_), false});
}

int NativeScriptTimedWaitRuntime::SetExecutionFlag(
    std::uint32_t queue_address) {
    if (queue_address != QueueAddress() || !Waiting() ||
        asGetActiveContext() != &context_ || context_.Suspend() < 0)
        throw std::runtime_error("PAL4 timed wait cannot suspend script");
    return 1;
}

void NativeScriptTimedWaitRuntime::SetClosedFlag(
    ScriptWaitHandlerState& state) {
    const auto found = std::find_if(handlers_.begin(), handlers_.end(),
        [&state](const Handler& handler) {
            return handler.state.get() == &state;
        });
    if (found == handlers_.end())
        throw std::logic_error("PAL4 timed wait handler is not registered");
    found->closed = true;
}

double NativeScriptTimedWaitRuntime::Remaining() const {
    return handlers_.empty() ? 0.0 : handlers_.front().state->remaining;
}

void NativeScriptTimedWaitRuntime::ClearHandlers() {
    if (asGetActiveContext() == &context_)
        throw std::logic_error("PAL4 cannot clear active timed wait handlers");
    pending_.reset();
    handlers_.clear();
}

bool NativeScriptTimedWaitRuntime::Tick(float elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0.0f)
        throw std::invalid_argument("PAL4 timed wait frame time is invalid");
    const bool had_handlers = Waiting();
    for (auto& handler : handlers_)
        handler_.Update(*handler.state, elapsed);
    std::erase_if(handlers_, [](const Handler& handler) {
        return handler.closed;
    });
    return had_handlers && !Waiting();
}

}  // namespace pal4
