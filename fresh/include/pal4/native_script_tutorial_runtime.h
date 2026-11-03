#pragma once
#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_tutorial_wait.h"
#include "pal4/script_poetry_wait.h"
#include <angelscript.h>
#include <functional>

namespace pal4 {
struct NativeTutorialServices {
    std::function<void(std::uint32_t)> show;
    std::function<std::uint32_t()> frame;
    std::function<bool(std::uint32_t)> visible;
};
// Tutorial and poetry use the same original 84966C frame-visibility wait.
// Each instance borrows one window service and one SDK context.
class NativeScriptTutorialRuntime final : public IScriptTutorialWaitRuntime,
    public IScriptPoetryWaitRuntime,
    public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptTutorialRuntime(asIScriptContext& context, NativeTutorialServices services);
    bool BlocksScript() const override { return active_; }
    bool Tick(float elapsed) override;
    void ClearHandlers();
    std::uint32_t RequestCount() const { return requests_; }
    std::uint32_t LastKey() const { return last_key_; }
    std::uint32_t UiFrameManager() override;
    void ShowTutorial(std::uint32_t manager, std::uint32_t key) override;
    void CreateAndHidePoetryWindow(std::uint32_t manager, std::uint32_t key) override {
        ShowTutorial(manager, key);
    }
    std::uint32_t PoetryWindow(std::uint32_t manager) override {
        return FindFrame(manager, ScriptTutorialWait::kFrameName);
    }
    std::uint32_t FindFrame(std::uint32_t manager, std::string_view name) override;
    TutorialWaitState* AllocateWait(std::uint32_t bytes) override;
    std::uint32_t ScriptExecutor() override;
    void ConstructWait(TutorialWaitState& state, std::uint8_t mode, std::uint32_t queue) override;
    void FlushWait(TutorialWaitState& state) override;
    std::uint32_t AddHandler(std::uint32_t queue, TutorialWaitState* state) override;
    bool IsVisible(std::uint32_t frame) override;
    void CloseWait(TutorialWaitState& state) override;
private:
    asIScriptContext& context_;
    NativeTutorialServices services_;
    ScriptTutorialWait tutorial_{*this};
    TutorialWaitState state_{};
    bool active_{};
    std::uint32_t requests_{}, last_key_{};
};
} // namespace pal4
