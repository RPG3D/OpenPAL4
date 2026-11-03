#include "pal4/pal_console_player_control_command.h"

#include "pal4/pal_console_native_commands.h"

#include <string>

namespace pal4 {

std::int32_t PalConsolePlayerControlCommand::Execute(
    const char* command, IPalConsolePlayerControlRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }

    const std::string name(*parser.Parse(command, 1));
    const auto id = runtime.PlayerId(runtime.PlayerManager(), name);
    if (id == 4) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }

    runtime.UpdatePlayerStates(runtime.PlayerManager(), id);
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadDword(manager + 0x2c50u);
    const auto player = manager + 2836u * index;
    runtime.ScriptOnRemove(runtime.ArenaManager(), player);
    runtime.SetStateFlag(player, 1u);
    runtime.ScriptOnAdd(runtime.ArenaManager(), player);
    runtime.SetActor(runtime.ActorControl(), player);
    return runtime.ResetAndSync(runtime.ActorControl(), 1u);
}

}  // namespace pal4
