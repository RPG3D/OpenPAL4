#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPlayerLockRuntime {
public:
    virtual ~IScriptPlayerLockRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetActionCode(std::uint32_t component,
                               std::string_view code) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void ResetAndSync(std::uint32_t control,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint8_t ProcessUiEvents(
        std::uint32_t manager, std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    virtual void UpdateGrid(std::uint32_t manager,
                            std::uint32_t position,
                            std::uint32_t direction) = 0;
};

class ScriptPlayerLockHandler final {
public:
    // IDA 0x5d3b10 and 0x5d3b70.
    [[nodiscard]] static std::uint8_t Lock(IScriptPlayerLockRuntime& runtime);
    static void Unlock(IScriptPlayerLockRuntime& runtime);
};

class IScriptNpcVisibilityRuntime {
public:
    virtual ~IScriptNpcVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t LeaveNpcScript(
        std::uint32_t arena, std::uint32_t npc) = 0;
    virtual void SetStateFlag(std::uint32_t npc, std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t EnterNpcScript(
        std::uint32_t arena, std::uint32_t npc) = 0;
};

class ScriptNpcVisibility final {
public:
    // IDA 0x5d3bc0. The native argument record supplies name at +4.
    [[nodiscard]] static std::uint32_t SetVisible(
        const char* name, bool visible, IScriptNpcVisibilityRuntime& runtime);
};

}  // namespace pal4
