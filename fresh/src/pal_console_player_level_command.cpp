#include "pal4/pal_console_player_level_command.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

std::int32_t Error(IPalConsolePlayerLevelRuntime& runtime,
                   std::string_view message) {
    const auto console = runtime.Console();
    return runtime.AddOutputLine(console, message);
}

void Output(IPalConsolePlayerLevelRuntime& runtime,
            const std::string& text) {
    const auto console = runtime.Console();
    (void)runtime.AddOutputLine(console, text);
}

}  // namespace

std::int32_t PalConsolePlayerLevelCommand::Execute(
    const char* command, IPalConsolePlayerLevelRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return Error(runtime, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");

    const std::string name(*parser.Parse(command, 1));
    const std::string level_text(*parser.Parse(command, 2));
    const auto manager = runtime.PlayerManager();
    const auto id = runtime.PlayerId(manager, name);
    if (id == 4)
        return Error(runtime, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");

    const auto second_manager = runtime.PlayerManager();
    const auto player = runtime.PlayerByIndex(second_manager, id);
    const auto level = std::atoi(level_text.c_str());
    if (level < 0 || level > 99)
        return Error(runtime,
                     "\xd6\xf7\xbd\xc7\xb5\xc8\xbc\xb6\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7");

    const auto component = player + 0x654u;
    runtime.SetComponentValue(component, 4u, level);
    runtime.InitializeFormatBuffer();
    bool records_ready = false;
    try {
        Output(runtime, runtime.PlayerName(player) +
                            " \xc9\xfd\xbc\xb6\xd6\xc1 " +
                            std::to_string(level) + " \xbc\xb6");
        runtime.UpdateLevelEvent(
            component, level, runtime.ReadDword(player + 0xa48u));
        const auto records = runtime.BuildNewMatchingRecords(
            player + 0x938u, level,
            runtime.ReadDword(player + 0xa48u));
        records_ready = true;
        for (const auto& record : records)
            Output(runtime, runtime.PlayerName(player) +
                                " \xd1\xa7\xbb\xe1\xcc\xd8\xbc\xbc " +
                                record);
        runtime.ReleaseMatchingRecords();
        records_ready = false;
    } catch (...) {
        if (records_ready) runtime.ReleaseMatchingRecords();
        (void)runtime.CleanupFormatBuffer();
        throw;
    }
    return runtime.CleanupFormatBuffer();
}

}  // namespace pal4
