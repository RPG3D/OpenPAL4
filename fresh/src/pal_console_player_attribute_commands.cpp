#include "pal4/pal_console_player_attribute_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <string>

namespace pal4 {
namespace {

std::int32_t Execute(const char* command, PalPlayerAttributeKind kind,
                     bool write, IPalConsolePlayerAttributeRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2) ||
        (write && !parser.Parse(command, 3))) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const std::string player_name(*parser.Parse(command, 1));
    const std::string attribute_name(*parser.Parse(command, 2));
    const std::string value = write ? std::string(*parser.Parse(command, 3)) :
                                      std::string();

    const auto id = runtime.PlayerId(runtime.PlayerManager(), player_name);
    if (id == 4) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), id);
    if (player == 0u) return 0;

    const auto collection = runtime.AttributeCollection(kind);
    const auto attribute = runtime.FindAttribute(collection, attribute_name);
    if (attribute == 0u) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb8\xc3\xca\xf4\xd0\xd4\xb2\xbb\xb4\xe6\xd4\xda");
    }

    const PalPlayerAttributeContext context{
        0x845560u, 0u,
        player + (kind == PalPlayerAttributeKind::Basic ? 0x658u : 0u)};
    if (write) runtime.SetAttribute(context, attribute, value);
    const std::string property(runtime.GetAttribute(context, attribute));
    const std::string display_name(runtime.PlayerName(player));
    runtime.InitializeFormatBuffer(display_name);
    try {
        const std::string output = display_name + " " + attribute_name + " :" + property;
        (void)runtime.AddOutputLine(runtime.Console(), output);
    } catch (...) {
        (void)runtime.CleanupFormatBuffer();
        throw;
    }
    return runtime.CleanupFormatBuffer();
}

}  // namespace

std::int32_t PalConsolePlayerAttributeCommands::GetPlayer(
    const char* command, IPalConsolePlayerAttributeRuntime& runtime) {
    return Execute(command, PalPlayerAttributeKind::Player, false, runtime);
}

std::int32_t PalConsolePlayerAttributeCommands::SetPlayer(
    const char* command, IPalConsolePlayerAttributeRuntime& runtime) {
    return Execute(command, PalPlayerAttributeKind::Player, true, runtime);
}

std::int32_t PalConsolePlayerAttributeCommands::GetBasic(
    const char* command, IPalConsolePlayerAttributeRuntime& runtime) {
    return Execute(command, PalPlayerAttributeKind::Basic, false, runtime);
}

std::int32_t PalConsolePlayerAttributeCommands::SetBasic(
    const char* command, IPalConsolePlayerAttributeRuntime& runtime) {
    return Execute(command, PalPlayerAttributeKind::Basic, true, runtime);
}

}  // namespace pal4
