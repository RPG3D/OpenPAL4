#pragma once

#include "pal4/minimap_scene_ui.h"
#include "pal4/timer_queue_entry.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Game-side ordering, separated from the legacy string-copy/library boundary.
// Native hosts can use std::string while reusing the same countdown policy.
class IMinimapCountdownStartRuntime {
public:
    virtual ~IMinimapCountdownStartRuntime() = default;
    virtual void RemoveCountdown(std::uint32_t owner) = 0;
    virtual void ScheduleCountdown(std::uint32_t owner) = 0;
    virtual void AssignCallback(std::uint32_t owner) = 0;
    virtual void ExecuteCountdown(std::uint32_t owner) = 0;
};
class IMinimapCountdownClearRuntime {
public:
    virtual ~IMinimapCountdownClearRuntime() = default;
    virtual void RemoveCountdown(std::uint32_t owner) = 0;
    virtual void ClearCallbackAndText(std::uint32_t owner) = 0;
};

class IMinimapSceneUiTimerRuntime {
public:
    virtual ~IMinimapSceneUiTimerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void RemoveTimer(std::uint32_t queue,
                             std::uint32_t identifier) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void WriteCallbackWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    virtual void CopyFloorAttribute(TimerQueueEntryState& entry,
                                    std::uint32_t source) = 0;
    virtual void InsertTimer(std::uint32_t queue,
                             std::uint32_t period,
                             std::uint32_t queue_cursor,
                             const TimerQueueEntryState& entry) = 0;
    virtual void AssignText(std::uint32_t destination,
                            const StringBufferState& source) = 0;
    virtual void ExecuteTimerScript(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::string_view EmptyErrorText() = 0;
    virtual void SetTextErrorFlag(MinimapSceneUiState& state,
                                  std::uint32_t buffer,
                                  std::uint8_t flag) = 0;
    virtual void ReallocateText(MinimapSceneUiState& state,
                                std::uint32_t buffer,
                                std::uint32_t length) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::u32string_view text) = 0;
    [[nodiscard]] virtual ITimerQueueEntryRuntime& EntryRuntime() = 0;
};

class MinimapSceneUiTimer final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843b20u;
    static constexpr std::uint32_t kExecuteFunction = 0x47caa0u;

    static void StartCountdown(MinimapSceneUiState& state, std::uint32_t owner,
        std::uint32_t period, IMinimapCountdownStartRuntime& runtime);
    static void ClearCountdown(MinimapSceneUiState& state, std::uint32_t owner,
        IMinimapCountdownClearRuntime& runtime);

    [[nodiscard]] static std::uint32_t Initialize(
        MinimapSceneUiState& state, std::uint32_t owner,
        std::uint32_t period, const StringBufferState& text,
        IMinimapSceneUiTimerRuntime& runtime);
    static void UpdateText(MinimapSceneUiState& state,
                           std::uint32_t owner,
                           IMinimapSceneUiTimerRuntime& runtime);
};

}  // namespace pal4
