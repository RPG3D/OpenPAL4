#include "pal4/pal_console_forbidden_skill_command.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {

std::int32_t PalConsoleForbiddenSkillCommand::Execute(
    const char* command, IPalConsoleForbiddenSkillRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2) ||
        !parser.Parse(command, 3)) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const std::string name(*parser.Parse(command, 1));
    const std::string skill_text(*parser.Parse(command, 2));
    const std::string enabled_text(*parser.Parse(command, 3));
    const auto skill = std::atoi(skill_text.c_str());
    const auto enabled = std::atoi(enabled_text.c_str()) > 0;

    const auto id = runtime.PlayerId(runtime.PlayerManager(), name);
    if (id == 4) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), id);
    if (player == 0u) return 0;
    const auto collection = player + 0x938u;
    runtime.SetPresent(collection, skill, enabled);
    const std::string player_name(runtime.PlayerName(player));
    runtime.InitializeFormatBuffer(player_name);

    std::uint32_t iterator = 0u;
    try {
        std::string output = player_name + " ForbidenSkill:";
        iterator = runtime.CreateIterator(collection);
        while (!runtime.IteratorDone(iterator)) {
            output += std::to_string(runtime.IteratorValue(iterator));
            output += "\t,";
            runtime.IteratorNext(iterator);
        }
        (void)runtime.AddOutputLine(runtime.Console(), output);
    } catch (...) {
        if (iterator != 0u) runtime.DestroyIterator(iterator, 1u);
        (void)runtime.CleanupFormatBuffer();
        throw;
    }
    if (iterator != 0u) runtime.DestroyIterator(iterator, 1u);
    return runtime.CleanupFormatBuffer();
}

}  // namespace pal4
