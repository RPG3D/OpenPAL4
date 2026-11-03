#pragma once

#include <cstdint>

namespace pal4 {

class IScriptUiTimerSaveDataRuntime {
public:
    virtual ~IScriptUiTimerSaveDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    [[nodiscard]] virtual std::uint32_t GameDataEntry(
        std::uint32_t game, std::uint32_t index) = 0;
};

class ScriptUiTimerSaveData final {
public:
    // IDA 0x5d8bd0, giUiTimerGetSaveData.
    [[nodiscard]] static std::uint32_t Get(
        IScriptUiTimerSaveDataRuntime& runtime);
};

}  // namespace pal4
