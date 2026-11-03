#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

std::uint8_t UpdatePlayerInventory(
    const char* command, std::uint32_t enabled,
    IPalConsoleNativeCommandRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        const auto console = runtime.Console();
        return static_cast<std::uint8_t>(runtime.AddOutputLine(
            console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd"));
    }
    const std::string name(*parser.Parse(command, 1));
    const auto manager = runtime.PlayerManager();
    const auto id = runtime.PlayerId(manager, name);
    if (id == 4) {
        const auto console = runtime.Console();
        return static_cast<std::uint8_t>(runtime.AddOutputLine(
            console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd"));
    }
    const auto second_manager = runtime.PlayerManager();
    const auto player = runtime.PlayerByIndex(second_manager, id);
    return runtime.UpdateInventoryState(player, enabled);
}

}  // namespace

std::optional<std::string_view> PalConsoleArgumentParser::Parse(
    const char* command, std::int32_t index) {
    if (command == nullptr || index <= 0) return std::nullopt;
    buffer_ = command;
    buffer_ += " end";
    std::size_t start = 0u;
    for (std::int32_t current = 1; current <= index; ++current) {
        const auto end = buffer_.find(' ', start);
        if (end == std::string::npos) return std::nullopt;
        if (current == index) {
            buffer_[end] = '\0';
            return std::string_view(buffer_.data() + start, end - start);
        }
        start = end + 1u;
    }
    return std::nullopt;
}

std::uint32_t PalConsoleNativeCommands::Exit(
    IPalConsoleNativeCommandRuntime& runtime) {
    const auto config = runtime.GameConfigAddress();
    runtime.WriteByte(config + 0x0cu, 1u);
    return config;
}

std::int32_t PalConsoleNativeCommands::SetPlayerPosition(
    const char* command, IPalConsoleNativeCommandRuntime& runtime) {
    PalConsoleArgumentParser parser;
    for (std::int32_t index = 1; index <= 4; ++index) {
        if (!parser.Parse(command, index)) {
            const auto console = runtime.Console();
            return runtime.AddOutputLine(console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
        }
    }

    std::array<std::string, 4> arguments;
    for (std::int32_t index = 1; index <= 4; ++index)
        arguments[static_cast<std::size_t>(index - 1)] =
            *parser.Parse(command, index);

    const auto manager = runtime.PlayerManager();
    const auto player_id = runtime.PlayerId(manager, arguments[0]);
    if (player_id == 4) {
        const auto console = runtime.Console();
        return runtime.AddOutputLine(console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }

    const auto second_manager = runtime.PlayerManager();
    const auto player = runtime.PlayerByIndex(second_manager, player_id);
    const std::array<float, 3> position{
        static_cast<float>(std::atoi(arguments[1].c_str())),
        static_cast<float>(std::atoi(arguments[2].c_str())),
        static_cast<float>(std::atoi(arguments[3].c_str()))};
    return runtime.SetPlayerPosition(player, position, 0u);
}

std::int32_t PalConsoleNativeCommands::SetGlobalVariable(
    const char* command, IPalConsoleNativeCommandRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        const auto console = runtime.Console();
        return runtime.AddOutputLine(console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const auto argument = *parser.Parse(command, 1);
    const std::string text(argument.substr(0u, 128u));
    const auto value = std::atoi(text.c_str());
    runtime.WriteDword(0x90dec0u, static_cast<std::uint32_t>(value));
    return value;
}

std::uint8_t PalConsoleNativeCommands::PlayerJoin(
    const char* command, IPalConsoleNativeCommandRuntime& runtime) {
    return UpdatePlayerInventory(command, 1u, runtime);
}

std::uint8_t PalConsoleNativeCommands::PlayerLeave(
    const char* command, IPalConsoleNativeCommandRuntime& runtime) {
    return UpdatePlayerInventory(command, 0u, runtime);
}

std::uint32_t PalConsoleNativeCommands::SetInventoryValue(
    const char* command, IPalConsoleNativeCommandRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        const auto console = runtime.Console();
        return static_cast<std::uint32_t>(runtime.AddOutputLine(
            console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd"));
    }
    const std::string text(*parser.Parse(command, 1));
    const auto value = std::atoi(text.c_str());
    if (value < 0) {
        const auto console = runtime.Console();
        return static_cast<std::uint32_t>(runtime.AddOutputLine(
            console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd"));
    }
    const auto inventory = runtime.InventorySubsystem();
    runtime.WriteDword(inventory + 0x134u,
                       static_cast<std::uint32_t>(value));
    return inventory;
}

}  // namespace pal4
