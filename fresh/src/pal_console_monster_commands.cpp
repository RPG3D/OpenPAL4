#include "pal4/pal_console_monster_commands.h"

namespace pal4 {

std::uint32_t PalConsoleMonsterCommands::RemoveCallback(
    std::uint32_t monster, IPalConsoleMonsterRuntime& runtime) {
    runtime.LeaveCharacter(runtime.ArenaManager(), monster);
    return monster;
}

std::int32_t PalConsoleMonsterCommands::KillAll(
    IPalConsoleMonsterRuntime& runtime) {
    runtime.IterateAndCallback(runtime.MonsterManager(),
                               &PalConsoleMonsterCommands::RemoveCallback,
                               0u);
    return runtime.Cleanup(runtime.MonsterManager());
}

}  // namespace pal4
