#include "pal4/pal_console_global_data_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

std::int32_t Output(IPalConsoleGlobalDataRuntime& runtime,
                    std::string_view format) {
    const auto console = runtime.Console();
    return runtime.AddOutputLine(console, format);
}

}  // namespace

std::int32_t PalConsoleGlobalDataCommands::SetGodMode(
    IPalConsoleGlobalDataRuntime& runtime) {
    runtime.InitializeInventoryGameData(runtime.Inventory());
    const auto inventory = runtime.Inventory();
    runtime.WriteDword(inventory + 0x134u, 0x1869fu);
    return Output(runtime,
                  "\xb5\xc0\xbe\xdf\xa3\xac\xd7\xb0\xb1\xb8\xa3\xac\xbd\xf0\xc7\xae\xcc\xed\xbc\xd3\xb3\xc9\xb9\xa6");
}

std::int32_t PalConsoleGlobalDataCommands::OpenMovieFlag(
    IPalConsoleGlobalDataRuntime& runtime) {
    const auto bitset = runtime.GlobalData() + 0x18u;
    for (std::uint32_t bit = 0u; bit < 32u; ++bit) {
        const auto address = bitset + 4u * (bit >> 5u);
        const auto value = runtime.ReadDword(address);
        runtime.WriteDword(address, value | (1u << (bit & 31u)));
    }
    return Output(runtime,
                  "\xbf\xaa\xb7\xc5\xcb\xf9\xd3\xd0\xb5\xe7\xd3\xb0\xb2\xa5\xb7\xc5\xc8\xa8\xcf\xde");
}

std::int32_t PalConsoleGlobalDataCommands::ResetGlobalData(
    IPalConsoleGlobalDataRuntime& runtime) {
    runtime.ResetDialogManager(runtime.GlobalData());
    runtime.SaveDialogSettings(runtime.GlobalData());
    (void)Output(runtime,
                 "\xd6\xd8\xd6\xc3\x20\xb5\xe7\xd3\xb0\xb2\xa5\xb7\xc5\xc8\xa8\xcf\xde\x2e\x2e\x2e\x20\x4f\x4b");
    (void)Output(runtime,
                 "\xd6\xd8\xd6\xc3\x20\xb0\xef\xd6\xfa\xce\xc4\xb1\xbe\xb2\xe9\xbf\xb4\xbc\xc7\xc2\xbc\x2e\x2e\x2e\x20\x4f\x4b");
    (void)Output(runtime,
                 "\xd6\xd8\xd6\xc3\x20\xd3\xce\xcf\xb7\xcd\xa8\xb9\xd8\xbc\xc7\xc2\xbc\x2e\x2e\x2e\x20\x4f\x4b");
    return Output(runtime,
                  "\xd6\xd8\xd6\xc3\x20\xcf\xc9\xbd\xa3\xce\xca\xb4\xf0\xb3\xc9\xbc\xa8\x2e\x2e\x2e\x20\x4f\x4b");
}

std::int32_t PalConsoleGlobalDataCommands::SetRoundTimes(
    const char* command, IPalConsoleGlobalDataRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1))
        return Output(runtime, "\xb4\xed\xce\xf3\xb2\xce\xca\xfd");

    const std::string text(*parser.Parse(command, 1));
    const auto times = std::atoi(text.c_str());
    const auto global = runtime.GlobalData();
    runtime.WriteDword(global + 0x28u,
                       static_cast<std::uint32_t>(times));
    runtime.SaveDialogSettings(runtime.GlobalData());
    runtime.InitializeFormatBuffer();
    try {
        const auto message = std::string(
            "\xc9\xe8\xb6\xa8\xd3\xce\xcf\xb7\xcd\xa8\xb9\xd8\x20") +
            std::to_string(times) +
            "\x20\xd6\xdc\xc4\xbf\x2e\x2e\x2e\x20\x4f\x4b";
        (void)Output(runtime, message);
    } catch (...) {
        (void)runtime.CleanupFormatBuffer();
        throw;
    }
    return runtime.CleanupFormatBuffer();
}

}  // namespace pal4
