#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// The 36-byte script wait handler installed by 0x5e7f90.
struct TutorialWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
    std::uint32_t frame{};
};
static_assert(sizeof(TutorialWaitState) == 0x24u);
static_assert(offsetof(TutorialWaitState, frame) == 0x20u);

// Native tutorial UI, frame lookup and script-queue calls.
class IScriptTutorialWaitRuntime {
public:
    virtual ~IScriptTutorialWaitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void ShowTutorial(std::uint32_t manager,
                              std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual TutorialWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(TutorialWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(TutorialWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, TutorialWaitState* state) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t frame) = 0;
    virtual void CloseWait(TutorialWaitState& state) = 0;
};

// Shows the tutorial dialog and waits for its frame to disappear.
class ScriptTutorialWait final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x84966cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::string_view kFrameName = "TutorialWindow";

    explicit ScriptTutorialWait(IScriptTutorialWaitRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e7f90: show tutorial key and queue a frame wait.
    [[nodiscard]] std::uint32_t Show(std::uint32_t key);
    // 0x5e8040: close wait when frame is null or invisible.
    void Update(TutorialWaitState& state);

private:
    IScriptTutorialWaitRuntime& runtime_;
};

}  // namespace pal4
