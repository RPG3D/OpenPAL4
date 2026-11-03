#include "pal4/pal_console_ui_unlock_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

void BadArguments(IPalConsoleUiUnlockRuntime& runtime) {
    (void)runtime.AddOutputLine(
        runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
}

void SetFixedEntry(const char* command, std::uint32_t entry,
                   IPalConsoleUiUnlockRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        BadArguments(runtime);
        return;
    }
    const std::string text(*parser.Parse(command, 1));
    runtime.SetGameDataEntry(runtime.GameInstance(), entry,
                             std::atoi(text.c_str()));
}

}  // namespace

void PalConsoleUiUnlockCommands::OpenSystemUi(
    const char* command, IPalConsoleUiUnlockRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2)) {
        BadArguments(runtime);
        return;
    }
    const std::string index_text(*parser.Parse(command, 1));
    const std::string value_text(*parser.Parse(command, 2));
    const auto index = std::atoi(index_text.c_str());
    if (index < 0 || index > 6) {
        BadArguments(runtime);
        return;
    }
    runtime.SetGameDataEntry(runtime.GameInstance(),
                             static_cast<std::uint32_t>(index + 8),
                             std::atoi(value_text.c_str()));
}

void PalConsoleUiUnlockCommands::OpenMagicUi(
    const char* command, IPalConsoleUiUnlockRuntime& runtime) {
    SetFixedEntry(command, 20u, runtime);
}

void PalConsoleUiUnlockCommands::OpenStuntUi(
    const char* command, IPalConsoleUiUnlockRuntime& runtime) {
    SetFixedEntry(command, 19u, runtime);
}

}  // namespace pal4
