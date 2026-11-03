#include "pal4/pal_console_player_stats_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

constexpr char kBadArguments[] =
    "\xb4\xed\xce\xf3\xb2\xce\xca\xfd";
constexpr char kHpOutOfRange[] =
    "\xd6\xf7\xbd\xc7\x48\x70\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7";
constexpr char kMpOutOfRange[] =
    "\xd6\xf7\xbd\xc7\x4d\x70\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7";
constexpr char kRageOutOfRange[] =
    "\xd6\xf7\xbd\xc7\x52\x61\x67\x65\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7";

std::int32_t Output(IPalConsolePlayerStatsRuntime& runtime,
                    std::string_view message) {
    const auto console = runtime.Console();
    return runtime.AddOutputLine(console, message);
}

std::int32_t SetValue(const char* command, std::uint32_t category,
                      std::string_view range_error,
                      IPalConsolePlayerStatsRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return Output(runtime, kBadArguments);

    const std::string name(*parser.Parse(command, 1));
    const std::string value_text(*parser.Parse(command, 2));
    const auto manager = runtime.PlayerManager();
    const auto id = runtime.PlayerId(manager, name);
    if (id == 4) return Output(runtime, kBadArguments);

    const auto second_manager = runtime.PlayerManager();
    const auto player = runtime.PlayerByIndex(second_manager, id);
    const auto value = std::atoi(value_text.c_str());
    if (value < 0) return Output(runtime, range_error);
    return runtime.SetComponentValue(player + 0x654u, category, value);
}

}  // namespace

std::int32_t PalConsolePlayerStatsCommands::SetHp(
    const char* command, IPalConsolePlayerStatsRuntime& runtime) {
    return SetValue(command, 7u, kHpOutOfRange, runtime);
}

std::int32_t PalConsolePlayerStatsCommands::SetMp(
    const char* command, IPalConsolePlayerStatsRuntime& runtime) {
    return SetValue(command, 9u, kMpOutOfRange, runtime);
}

std::int32_t PalConsolePlayerStatsCommands::SetRage(
    const char* command, IPalConsolePlayerStatsRuntime& runtime) {
    return SetValue(command, 8u, kRageOutOfRange, runtime);
}

}  // namespace pal4
