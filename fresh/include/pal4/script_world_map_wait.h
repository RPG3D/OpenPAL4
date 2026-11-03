#pragma once

#include "pal4/script_tutorial_wait.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Native world-map UI event, script queue and state-transition operations.
class IScriptWorldMapWaitRuntime {
public:
    virtual ~IScriptWorldMapWaitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void ProcessWorldMapEvent(std::uint32_t manager,
                                      std::uint32_t first,
                                      std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual TutorialWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(TutorialWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(TutorialWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            TutorialWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CreateDocument() = 0;
    virtual void EnterState(std::uint32_t state,
                            std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDocument(
        std::uint32_t document) = 0;
};

// Opens world map state eleven and queues the shared frame wait handler.
class ScriptWorldMapWait final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x84966cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::string_view kFrameName = "WorldMap";

    explicit ScriptWorldMapWait(IScriptWorldMapWaitRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e8060: process map event, queue frame wait, enter state eleven.
    [[nodiscard]] std::uint32_t Show();

private:
    IScriptWorldMapWaitRuntime& runtime_;
};

}  // namespace pal4
