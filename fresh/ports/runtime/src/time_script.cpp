#include "pal4/portable/time_script.h"
#include "pal4/portable/script_stack.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
void Require(bool value, const char* message) {
    if (!value) throw std::logic_error(message);
}
}

TimeScript::TimeScript(Dispatch dispatch) : dispatch_(std::move(dispatch)) {
    Require(bool(dispatch_), "time script dispatcher is missing");
}

void TimeScript::Start(float delay_seconds, std::string_view callback) {
    Require(std::isfinite(delay_seconds), "invalid time script delay");
    Require(callback.find('\0') == std::string_view::npos,
        "NUL in time script callback");
    delay_ = delay_seconds;
    elapsed_ = 0.0f;
    callback_.assign(callback);
    active_ = true;
}

void TimeScript::Terminate() {
    delay_ = 0.0f;
    elapsed_ = 0.0f;
    callback_.clear();
    active_ = false;
}

bool TimeScript::Advance(float elapsed_seconds, bool actor_control_active,
    bool script_executor_busy) {
    Require(std::isfinite(elapsed_seconds) && elapsed_seconds >= 0.0f,
        "invalid time script frame time");
    if (!active_) return false;

    const auto total = static_cast<double>(elapsed_seconds) + elapsed_;
    elapsed_ = static_cast<float>(total);
    if (!(total >= delay_)) return false;

    elapsed_ = 0.0f;
    if (!actor_control_active && script_executor_busy) return true;

    const auto callback = callback_;
    dispatch_(callback);
    return true;
}

bool TimeScript::Active() const { return active_; }
float TimeScript::Elapsed() const { return elapsed_; }
float TimeScript::Delay() const { return delay_; }
std::string_view TimeScript::Callback() const { return callback_; }

std::optional<std::uint64_t> PushTimeScriptCallback(ScriptStack& stack,
    std::string_view module, std::string_view declaration,
    IScriptContextServices& services) {
    if(stack.ContainsActive(module,declaration))return {};
    const auto context=stack.Push(module,declaration,services);
    if(!context)throw std::runtime_error("time script callback entry is missing: "+std::string(declaration));
    return context;
}

} // namespace pal4::portable
