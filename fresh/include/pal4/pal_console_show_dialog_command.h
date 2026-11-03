#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleShowDialogRuntime {
public:
    virtual ~IPalConsoleShowDialogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::uint32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateScenarioDialog(
        std::uint32_t manager) = 0;
    // Zero means the native text-buffer allocation failed.
    [[nodiscard]] virtual std::uint32_t CreateTextBuffer(
        std::string_view text) = 0;
    virtual void SetDialogText(std::uint32_t dialog,
                               std::uint32_t text_buffer,
                               std::uint32_t editbox_value) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseTextBuffer(
        std::uint32_t text_buffer) = 0;
};

class PalConsoleShowDialogCommand final {
public:
    // IDA 0x5bb8f0, command "showDialog".
    [[nodiscard]] static std::uint32_t Execute(
        const char* command, IPalConsoleShowDialogRuntime& runtime);
};

}  // namespace pal4
