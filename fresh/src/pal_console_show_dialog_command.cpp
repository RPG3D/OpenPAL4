#include "pal4/pal_console_show_dialog_command.h"

#include "pal4/pal_console_native_commands.h"

#include <algorithm>
#include <string>

namespace pal4 {

std::uint32_t PalConsoleShowDialogCommand::Execute(
    const char* command, IPalConsoleShowDialogRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        const auto console = runtime.Console();
        return runtime.AddOutputLine(console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }

    const auto argument = *parser.Parse(command, 1);
    const std::string text(argument.substr(0u, std::min<std::size_t>(
                                               argument.size(), 128u)));
    const auto manager = runtime.UiFrameManager();
    const auto dialog = runtime.CreateScenarioDialog(manager);
    const auto buffer = runtime.CreateTextBuffer(text);
    try {
        runtime.SetDialogText(dialog, buffer, 0x3dcccccdU);
    } catch (...) {
        if (buffer != 0u) (void)runtime.ReleaseTextBuffer(buffer);
        throw;
    }
    return buffer != 0u ? runtime.ReleaseTextBuffer(buffer) : 0u;
}

}  // namespace pal4
