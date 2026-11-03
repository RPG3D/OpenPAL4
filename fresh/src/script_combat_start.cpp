#include "pal4/script_combat_start.h"

namespace pal4 {
namespace {

struct DocumentOwner {
    IScriptCombatStartRuntime& runtime;
    bool active{true};

    ~DocumentOwner() {
        if (active) (void)runtime.DestroyDocument();
    }

    std::uint32_t Close() noexcept {
        active = false;
        return runtime.DestroyDocument();
    }
};

}  // namespace

std::uint32_t ScriptCombatStarter::Start(
    const char* world, IScriptCombatStartRuntime& runtime) {
    auto& initial = runtime.ScriptExecutor();
    if (initial.commands.Capacity() == 0u) return initial.address;
    if (initial.commands.Commands().empty()) return 0u;

    runtime.CreateDocument("CombatParam");
    DocumentOwner document{runtime};
    const std::string_view combat_world = world == nullptr ? "" : world;
    runtime.SetCombatWorld("CombatWorld", combat_world, 0u, 1u);

    for (std::size_t i = 0;; ++i) {
        const auto& commands = runtime.ScriptExecutor().commands.Commands();
        if (i >= commands.size()) break;
        const auto value = runtime.ScriptExecutor().commands.Commands()[i].argument;
        const auto id = runtime.ScriptExecutor().commands.Commands()[i].command;
        runtime.SetCombatMonster(id, value, 0u);
    }

    const auto count = runtime.ScriptExecutor().commands.Commands().size();
    runtime.LogStartCombat(0x339u, "none",
                           static_cast<std::uint32_t>(count), combat_world);
    runtime.EnterGameState(runtime.GameInstance(), 3u);
    runtime.ScriptExecutor().commands.Clear();
    return document.Close();
}

}  // namespace pal4
