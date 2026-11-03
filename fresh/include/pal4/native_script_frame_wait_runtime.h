#pragma once

namespace pal4 {

class INativeScriptFrameWaitRuntime {
public:
    virtual ~INativeScriptFrameWaitRuntime() = default;
    [[nodiscard]] virtual bool BlocksScript() const { return false; }
    [[nodiscard]] virtual bool Tick(float elapsed) = 0;
};

// Tick processes waits/VM progress but never advances the shared screen fade.
// State owners such as Logo schedule screen advancement after these handlers.
class INativeScriptHandlerWaitRuntime : public INativeScriptFrameWaitRuntime {
public:
    ~INativeScriptHandlerWaitRuntime() override = default;
};

}  // namespace pal4
