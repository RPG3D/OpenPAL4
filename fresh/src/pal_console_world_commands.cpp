#include "pal4/pal_console_world_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

std::int32_t BadArguments(IPalConsoleWorldRuntime& runtime) {
    return runtime.AddOutputLine(
        runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
}

}  // namespace

std::int32_t PalConsoleWorldCommands::SetMazeVariable(
    const char* command, IPalConsoleWorldRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return BadArguments(runtime);

    const std::string index_text(*parser.Parse(command, 1));
    const std::string value_text(*parser.Parse(command, 2));
    const auto index = std::atoi(index_text.c_str());
    if (index < 0 || index > 4) return BadArguments(runtime);
    const auto value = std::atoi(value_text.c_str());
    runtime.WriteDword(0x90dec4u + 4u * static_cast<std::uint32_t>(index),
                       static_cast<std::uint32_t>(value));
    return value;
}

std::int32_t PalConsoleWorldCommands::OpenWeather(
    const char* command, IPalConsoleWorldRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) return BadArguments(runtime);
    const std::string value_text(*parser.Parse(command, 1));
    const auto arena = runtime.ArenaManager();
    return runtime.SelectWeatherEffect(arena,
                                       std::atoi(value_text.c_str()));
}

std::uint8_t PalConsoleWorldCommands::CloseWeather(
    IPalConsoleWorldRuntime& runtime) {
    return runtime.UpdateWeatherEffects(runtime.ArenaManager());
}

}  // namespace pal4
