#include "pal4/pal_console_puzzle_command.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

void StoreInt(std::uint32_t document, std::string_view key,
              std::int32_t value, IPalConsolePuzzleRuntime& runtime) {
    const auto found = runtime.FindTreeAttribute(document, key);
    if (found) {
        runtime.SetInt(*found, value);
        return;
    }
    const auto attribute = runtime.InitializeIntAttribute(key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        throw;
    }
    runtime.DestroyAttribute(attribute);
}

std::int32_t Start(const char* command, std::int32_t mode,
                   std::int32_t type_limit,
                   IPalConsolePuzzleRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) {
        return runtime.AddOutputLine(
            runtime.Console(), "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
    }
    const std::string text(*parser.Parse(command, 1));
    const auto type = std::atoi(text.c_str());
    if (type >= type_limit) {
        return runtime.AddOutputLine(
            runtime.Console(),
            "\xd7\xd3\xd3\xce\xcf\xb7\xc0\xe0\xd0\xcd\xb3\xac\xb9\xfd\xc9\xcf\xcf\xde");
    }

    const auto document = runtime.InitializeXmlDocument();
    try {
        StoreInt(document, "Mode", mode, runtime);
        StoreInt(document, "Type", type, runtime);
        runtime.ClearConsole(runtime.Console());
        runtime.InitializeStateAndUi(runtime.GameInstance(), 8u, document);
    } catch (...) {
        (void)runtime.DestroyXmlDocument(document);
        throw;
    }
    return runtime.DestroyXmlDocument(document);
}

}  // namespace

std::int32_t PalConsolePuzzleCommand::Execute(
    const char* command, IPalConsolePuzzleRuntime& runtime) {
    return Start(command, 0, 11, runtime);
}

std::int32_t PalConsolePuzzleCommand::StartJigsaw(
    const char* command, IPalConsolePuzzleRuntime& runtime) {
    return Start(command, 1, 13, runtime);
}

}  // namespace pal4
