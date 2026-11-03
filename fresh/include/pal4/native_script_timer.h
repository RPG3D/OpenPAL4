#pragma once
#include <memory>
#include <string_view>

#include "pal4/pal_script_executor.h"
#include "pal4/script_timed_state.h"
namespace pal4 {
class INativeScriptTimerHost {
   public:
    virtual ~INativeScriptTimerHost() = default;
    virtual bool ActorControlActive() = 0;
    virtual bool ScriptBusy() = 0;
    virtual bool HasStackModule(std::string_view) = 0;
    virtual std::string_view ScriptSource() = 0;
    virtual void LoadTimerModule(std::string_view source, std::string_view module) = 0;
    virtual void PushTimerModule(std::string_view module, std::string_view function) = 0;
};
// Owns the timer fields and callback text, not the full legacy script executor.
// The shared processor owns timer/gate policy; the host owns SDK modules/stack.
class NativeScriptTimer final : public IScriptTimedStateRuntime {
   public:
    NativeScriptTimer();
    ~NativeScriptTimer();
    void Process(float elapsed, INativeScriptTimerHost&);
    void Reset();
    // IDA60035A: scene executor reset disables future triggers while retaining
    // callback/delay/elapsed and any already executing timed context.
    void DisableForSceneRelease();
    bool Enabled() const;
    float Elapsed() const;
    float Delay() const;
    std::string_view Callback() const;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t EmptyString() override;
    std::uint32_t ScriptExecutor() override;
    std::uint32_t InitializeState(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t) override;
    std::uint32_t TextLength(std::uint32_t) override;
    std::uint32_t InitializeText(std::uint32_t, std::uint32_t, std::uint32_t) override;

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace pal4
