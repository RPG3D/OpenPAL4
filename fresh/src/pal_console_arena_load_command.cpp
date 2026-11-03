#include "pal4/pal_console_arena_load_command.h"

#include "pal4/pal_console_native_commands.h"

#include <array>
#include <string>

namespace pal4 {
namespace {

void StoreInt(std::uint32_t document, std::string_view key,
              std::int32_t value, bool tree_lookup,
              IPalConsoleArenaLoadRuntime& runtime) {
    const auto found = tree_lookup
        ? runtime.FindTreeAttribute(document, key)
        : runtime.FindConfigAttribute(document, key);
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

void StoreString(std::uint32_t document, std::string_view key,
                 std::string_view value, bool tree_lookup,
                 IPalConsoleArenaLoadRuntime& runtime) {
    const auto found = tree_lookup
        ? runtime.FindTreeAttribute(document, key)
        : runtime.FindConfigAttribute(document, key);
    if (found) {
        runtime.SetString(*found, value);
        return;
    }
    const auto attribute = runtime.InitializeStringAttribute(key, value);
    try {
        runtime.AddAttribute(document, attribute);
    } catch (...) {
        runtime.DestroyAttribute(attribute);
        throw;
    }
    runtime.DestroyAttribute(attribute);
}

}  // namespace

std::int32_t PalConsoleArenaLoadCommand::Execute(
    const char* command, IPalConsoleArenaLoadRuntime& runtime) {
    PalConsoleArgumentParser parser;
    for (std::int32_t index = 1; index <= 3; ++index) {
        if (!parser.Parse(command, index)) {
            const auto console = runtime.Console();
            return runtime.AddOutputLine(console, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");
        }
    }

    std::array<std::string, 3> arguments;
    for (std::int32_t index = 1; index <= 3; ++index)
        arguments[static_cast<std::size_t>(index - 1)] =
            *parser.Parse(command, index);

    const auto document = runtime.InitializeXmlDocument();
    try {
        StoreInt(document, "PALGAMEIV_LOADING_TYPE", 0, true, runtime);
        StoreString(document, "PALGAMEIV_LOADING_ARENA", arguments[0],
                    true, runtime);
        StoreString(document, "PALGAMEIV_LOADING_AREA", arguments[1],
                    true, runtime);
        StoreString(document, "PALGAMEIV_LOADING_AREA_RES", arguments[2],
                    false, runtime);
        StoreInt(document, "PALGAMEIV_LOADING_AUTOREADY", 1,
                 false, runtime);
        const auto game = runtime.GameInstance();
        runtime.InitializeStateAndUi(game, 6u, document);
    } catch (...) {
        (void)runtime.DestroyXmlDocument(document);
        throw;
    }
    return runtime.DestroyXmlDocument(document);
}

}  // namespace pal4
