#pragma once

#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>

namespace pal4 {

class IScriptUiTimerRuntime {
public:
    virtual ~IScriptUiTimerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint8_t ReadReference(
        std::uint32_t data) = 0;
    virtual void WriteReference(std::uint32_t data,
                                std::uint8_t value) = 0;
    virtual void SetErrorFlag(UiFrameTimerText& text,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(UiFrameTimerText& text,
                                             std::uint32_t length,
                                             std::uint8_t flag) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void SetTextSize(UiFrameTimerText& text,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTimerFrame(
        std::uint32_t manager, std::uint32_t period,
        const UiFrameTimerText& text) = 0;
};

class ScriptUiTimer final {
public:
    // IDA 0x5d7e80, giStartUiTimer(int, string&).
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t period, const UiFrameTimerText& source,
        IScriptUiTimerRuntime& runtime);
};

}  // namespace pal4
