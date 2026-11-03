#pragma once

#include "pal4/script_command_queue.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScriptCombatExecutor {
    std::uint32_t address{};
    ScriptCommandQueue commands;
};

class IScriptCombatStartRuntime {
public:
    virtual ~IScriptCombatStartRuntime() = default;
    [[nodiscard]] virtual ScriptCombatExecutor& ScriptExecutor() = 0;
    virtual void CreateDocument(std::string_view root) = 0;
    virtual void SetCombatWorld(std::string_view key, std::string_view world,
                                std::uint32_t mode, std::uint32_t count) = 0;
    virtual void SetCombatMonster(std::uint32_t id, std::uint32_t value,
                                  std::uint32_t group) = 0;
    virtual void LogStartCombat(std::uint32_t source_line,
                                std::string_view placeholder,
                                std::uint32_t monster_count,
                                std::string_view world) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void EnterGameState(std::uint32_t game, std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDocument() noexcept = 0;
};

class ScriptCombatStarter final {
public:
    // IDA 0x5d44a0, giStartCombat.
    [[nodiscard]] static std::uint32_t Start(
        const char* world, IScriptCombatStartRuntime& runtime);
};

}  // namespace pal4
