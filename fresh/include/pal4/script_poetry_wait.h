#pragma once

#include "pal4/script_tutorial_wait.h"

#include <cstdint>

namespace pal4 {

// Native poetry-window and script-queue operations.
class IScriptPoetryWaitRuntime {
public:
    virtual ~IScriptPoetryWaitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void CreateAndHidePoetryWindow(std::uint32_t manager,
                                           std::uint32_t poetry_id) = 0;
    [[nodiscard]] virtual std::uint32_t PoetryWindow(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual TutorialWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(TutorialWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(TutorialWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, TutorialWaitState* state) = 0;
};

// Shows a poetry window and optionally waits for its visibility to end.
class ScriptPoetryWait final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x84966cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptPoetryWait(IScriptPoetryWaitRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e8800: return frame directly or enqueue shared frame wait.
    [[nodiscard]] std::uint32_t Show(std::uint32_t poetry_id,
                                     std::uint32_t wait_mode);

private:
    IScriptPoetryWaitRuntime& runtime_;
};

}  // namespace pal4
