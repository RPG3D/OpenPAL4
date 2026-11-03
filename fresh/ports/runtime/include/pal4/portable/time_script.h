#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace pal4::portable {
class ScriptStack;
class IScriptContextServices;

// One original giTimeScript timer, advanced once per game frame.
class TimeScript {
public:
    using Dispatch = std::function<void(std::string_view)>;

    explicit TimeScript(Dispatch dispatch);
    void Start(float delay_seconds, std::string_view callback);
    void Terminate();
    bool Advance(float elapsed_seconds, bool actor_control_active,
        bool script_executor_busy);
    bool Active() const;
    float Elapsed() const;
    float Delay() const;
    std::string_view Callback() const;

private:
    Dispatch dispatch_;
    std::string callback_;
    float delay_{};
    float elapsed_{};
    bool active_{};
};

// Returns no context for an already active callback; a missing declaration is
// an error because the original timed-script runner cannot prepare it either.
std::optional<std::uint64_t> PushTimeScriptCallback(ScriptStack& stack,
    std::string_view module, std::string_view declaration,
    IScriptContextServices& services);

} // namespace pal4::portable
