#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {
// Modern script-string boundary. The host owns callback text and delegates the
// game policy to MinimapSceneUiTimer and ScriptUiTimerControl.
class IScriptUiCountdown {
public:
    virtual ~IScriptUiCountdown() = default;
    virtual void Start(std::uint32_t seconds, std::string_view callback) = 0;
    virtual void Clear() = 0;
    [[nodiscard]] virtual std::uint32_t Pause() = 0;
    virtual void Resume() = 0;
};
}  // namespace pal4
