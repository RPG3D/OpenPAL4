#pragma once

#include <cstdint>

namespace pal4 {

class IScriptScenarioPortraitRuntime {
public:
    virtual ~IScriptScenarioPortraitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ScenarioDialog(
        std::uint32_t frames) = 0;
    [[nodiscard]] virtual std::uint8_t IsVisible(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint8_t SetPortrait(
        std::uint32_t dialog, std::uint32_t name,
        std::int32_t argument) = 0;
};

class ScriptScenarioPortrait final {
public:
    // IDA 0x5d5760, giSetPortrait.
    [[nodiscard]] static std::uint8_t Set(
        std::uint32_t name_payload, std::int32_t argument,
        IScriptScenarioPortraitRuntime& runtime);
};

}  // namespace pal4
