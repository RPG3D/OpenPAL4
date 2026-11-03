#include "pal4/pal_console_actor_control_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {

std::uint32_t PalConsoleActorControlCommands::JumpUnlock(
    IPalConsoleActorControlRuntime& runtime) {
    const auto control = runtime.ActorControl();
    runtime.WriteDword(control + 0x110u, 1u);
    return control;
}

std::int32_t PalConsoleActorControlCommands::SetControlMode(
    const char* command, IPalConsoleActorControlRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const std::string text(*parser.Parse(command, 1));
    const auto value = std::atoi(text.c_str());
    return runtime.SetWord28(runtime.ActorControl(), value == 1 ? 0u : 1u);
}

}  // namespace pal4
