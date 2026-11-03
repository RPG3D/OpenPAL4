#pragma once

#include "pal4/native_script_frame_runner.h"
#include "pal4/script_flash_effect.h"

#include <angelscript.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace pal4 {

class IEffectRenderRuntime;
struct PalScreenEffectState;

// Owns flash waits for one context. Scene Tick advances the fade first;
// Handlers() observes completion without changing fade time.
class NativeScriptFlashRuntime final : public IScriptFlashRuntime,
                                       public INativeScriptFrameWaitRuntime {
public:
    NativeScriptFlashRuntime(PalScreenEffectState& effect,
                             IEffectRenderRuntime& render,
                             asIScriptContext& context);
    ~NativeScriptFlashRuntime() override;
    NativeScriptFlashRuntime(const NativeScriptFlashRuntime&) = delete;
    NativeScriptFlashRuntime& operator=(const NativeScriptFlashRuntime&) = delete;

    [[nodiscard]] ScriptFlashEffect& Flash() { return flash_; }
    [[nodiscard]] bool Waiting() const { return !handlers_.empty(); }
    [[nodiscard]] bool BlocksScript() const override;
    void ClearHandlers();
    void UseWorldClock(bool enabled) noexcept { world_clock_ = enabled; }
    // Combined scene tick; video-state screen updates must use Handlers().
    [[nodiscard]] bool Tick(float elapsed) override;
    [[nodiscard]] bool ProcessWaitHandlers();
    class HandlerRuntime final : public INativeScriptHandlerWaitRuntime {
    public:
        explicit HandlerRuntime(NativeScriptFlashRuntime& owner) : owner_(owner) {}
        bool BlocksScript() const override { return owner_.BlocksScript(); }
        bool Tick(float elapsed) override;
    private:
        NativeScriptFlashRuntime& owner_;
    };
    [[nodiscard]] INativeScriptHandlerWaitRuntime& Handlers() { return handler_runtime_; }

    [[nodiscard]] std::uint32_t ScreenEffect() override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void StartFade(std::uint32_t effect, float duration,
                   std::uint32_t hold, FlashColor color,
                   float alpha_start, float alpha_end,
                   float alpha_on_finish) override;
    [[nodiscard]] FlashWaitState* Allocate(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    void Construct(FlashWaitState& state, std::uint8_t wait_mode,
                   std::uint32_t queue) override;
    void Flush(FlashWaitState& state) override;
    void Close(FlashWaitState& state) override;
    [[nodiscard]] std::uint32_t AddHandler(
        std::uint32_t queue, FlashWaitState* state) override;

private:
    bool world_clock_{};
    struct Handler {
        std::unique_ptr<FlashWaitState> state;
        bool closed{};
        bool blocking{};
    };

    [[nodiscard]] std::uint32_t QueueAddress() const;

    PalScreenEffectState& effect_;
    IEffectRenderRuntime& render_;
    asIScriptContext& context_;
    ScriptFlashEffect flash_;
    std::unique_ptr<FlashWaitState> pending_;
    std::uint8_t pending_wait_mode_{};
    std::vector<Handler> handlers_;
    HandlerRuntime handler_runtime_{*this};
};

}  // namespace pal4
