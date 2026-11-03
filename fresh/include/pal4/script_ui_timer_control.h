#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptUiTimerControlRuntime {
public:
    virtual ~IScriptUiTimerControlRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void ClearTimerAndText(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveTask(
        std::uint32_t queue, std::uint32_t identifier) = 0;
    virtual void CreateTask(std::uint32_t queue,
                            std::uint32_t identifier,
                            std::uint32_t period_bits,
                            std::uint32_t context,
                            std::uint32_t callback,
                            std::uint32_t option) = 0;
};

class ScriptUiTimerControl final {
public:
    // IDA 0x5d8b60, giClearUiTimer.
    static void Clear(IScriptUiTimerControlRuntime& runtime);
    // IDA 0x5d8b70, giPauseUiTimer.
    [[nodiscard]] static std::uint32_t Pause(
        IScriptUiTimerControlRuntime& runtime);
    // IDA 0x5d8b80, giResumeUiTimer.
    static void Resume(IScriptUiTimerControlRuntime& runtime);
};

}  // namespace pal4
