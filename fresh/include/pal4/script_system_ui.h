#pragma once

#include <cstdint>

namespace pal4 {

class IScriptSystemUiRuntime {
public:
    virtual ~IScriptSystemUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void SetGameDataEntry(std::uint32_t game,
                                  std::uint32_t entry,
                                  std::uint8_t value) = 0;
    virtual void InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t event_tree,
                              std::uint32_t event_descriptor,
                              std::uint32_t menu_index,
                              std::uint32_t reserved) = 0;
    virtual void SetFrameFlag(std::uint32_t frames,
                              std::uint8_t enabled) = 0;
};

class ScriptSystemUi final {
public:
    // IDA 0x5d6d20, giGrantSystemUi.
    static void Grant(std::uint32_t index, std::uint8_t enabled,
                      IScriptSystemUiRuntime& runtime);
    // IDA 0x5d6d50, giOpenSystemUi.
    static void Open(std::uint32_t index, IScriptSystemUiRuntime& runtime);
    // IDA 0x5d6da0, giGrantSmithSystem.
    static void GrantSmith(std::uint8_t first, std::uint8_t second,
                           std::uint8_t third, std::uint8_t fourth,
                           IScriptSystemUiRuntime& runtime);
    // IDA 0x5d6e10, giGrantMagicSystem.
    static void GrantMagic(std::uint8_t first, std::uint8_t second,
                           IScriptSystemUiRuntime& runtime);
    // IDA 0x5d7af0, giArenaSkillEnable.
    static void SetArenaSkillEnabled(
        std::uint8_t enabled, IScriptSystemUiRuntime& runtime);
};

}  // namespace pal4
