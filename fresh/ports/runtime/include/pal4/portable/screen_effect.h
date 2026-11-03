#pragma once
#include "pal4/portable/script_stack.h"

namespace pal4::portable {
struct ScreenFadeState {
    std::uint8_t red{},green{},blue{};
    float elapsed{},duration{},alpha{},alpha_start{},alpha_end{},alpha_on_finish{};
    std::uint32_t phase{1},keep_active{},active{};
};
class ScreenEffectServices final:public IScriptContextServices {
public:
    explicit ScreenEffectServices(IScriptContextServices& next):next_(next) {}
    void OnCall(const ScriptRequest& request) override {next_.OnCall(request);}
    ScriptResponse Invoke(const ScriptRequest& request) override;
    // Completion is delivered after the visual timeline has finished.
    std::vector<ScriptToken> Advance(float elapsed);
    const ScreenFadeState& State() const {return state_;}
    void Reset();
private:
    IScriptContextServices& next_;
    ScreenFadeState state_;
    std::vector<ScriptToken> pending_;
};
} // namespace pal4::portable
