#pragma once

#include "pal4/minimap_scene_ui_timer.h"
#include "pal4/script_ui_countdown.h"
#include "pal4/script_ui_timer_control.h"

#include <functional>
#include <string>

namespace pal4 {
class NativeGameTimerQueue;

// Borrows the registered portrait state, manager, queue and synchronous UI/script
// endpoints. Destroy before retiring those owners. Native callback text uses
// std::string; legacy string-copy/destruction helpers must not touch its fields.
class NativeUiCountdown final : public IScriptUiCountdown,
    private IScriptUiTimerControlRuntime, private IMinimapCountdownClearRuntime {
public:
    NativeUiCountdown(MinimapSceneUiState& state, std::uint32_t owner,
        std::uint32_t manager, NativeGameTimerQueue& timers,
        std::function<void()> execute,
        std::function<void(std::uint32_t,std::string_view)> set_text);
    ~NativeUiCountdown() override;
    NativeUiCountdown(const NativeUiCountdown&) = delete;
    NativeUiCountdown& operator=(const NativeUiCountdown&) = delete;

    void Start(std::uint32_t seconds, std::string_view callback) override;
    void Clear() override;
    [[nodiscard]] std::uint32_t Pause() override;
    void Resume() override;
    void Tick();

private:
    void Owner(std::uint32_t owner) const;
    [[nodiscard]] std::string_view Name() const;
    void SyncCallback();
    void ScheduleStart(std::uint32_t owner);
    void RemoveCountdown(std::uint32_t owner) override;
    void ClearCallbackAndText(std::uint32_t owner) override;
    std::uint32_t FrameManager() override;
    std::uint32_t FindFrame(std::uint32_t manager,std::string_view name) override;
    void ClearTimerAndText(std::uint32_t owner) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t RemoveTask(std::uint32_t queue,std::uint32_t identifier) override;
    void CreateTask(std::uint32_t queue,std::uint32_t identifier,
        std::uint32_t period_bits,std::uint32_t context,
        std::uint32_t callback,std::uint32_t option) override;

    MinimapSceneUiState& state_;
    std::uint32_t owner_,manager_;
    NativeGameTimerQueue& timers_;
    std::function<void()> execute_;
    std::function<void(std::uint32_t,std::string_view)> set_text_;
    std::string callback_;
};
}  // namespace pal4
