#include "pal4/pal_console_state_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <algorithm>
#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

bool ParseValue(const char* command, std::int32_t& value) {
    PalConsoleArgumentParser parser;
    const auto field = parser.Parse(command, 1);
    if (!field) return false;
    const std::string raw(*field);
    value = std::atoi(raw.c_str());
    return true;
}

std::int32_t MissingArgument(IPalConsoleStateRuntime& runtime) {
    return runtime.AddOutputLine(
        runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
}

}  // namespace

std::int32_t PalConsoleStateCommands::SetPetShow(
    const char* command, IPalConsoleStateRuntime& runtime) {
    std::int32_t value = 0;
    if (!ParseValue(command, value)) return MissingArgument(runtime);
    const auto manager = runtime.PetManager();
    runtime.WriteDword(manager + 0x654u,
                       static_cast<std::uint32_t>(std::clamp(value, 0, 1)));
    return static_cast<std::int32_t>(manager);
}

std::int32_t PalConsoleStateCommands::EnableSceneTransition(
    const char* command, IPalConsoleStateRuntime& runtime) {
    std::int32_t value = 0;
    if (!ParseValue(command, value)) return MissingArgument(runtime);
    const auto actor = runtime.ActorControl();
    return runtime.ActivateStatusMode(actor, value);
}

std::int32_t PalConsoleStateCommands::ResetUiTimer(
    const char* command, IPalConsoleStateRuntime& runtime) {
    std::int32_t value = 0;
    if (!ParseValue(command, value)) return MissingArgument(runtime);
    const auto frame = runtime.UiFrameManager();
    return runtime.SetTimerFrameDuration(frame, value);
}

}  // namespace pal4
